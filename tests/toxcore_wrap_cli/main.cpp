#include "core/toxcore_wrapper.h"

#include <toxcore/tox.h>

#include <algorithm>
#include <charconv>
#include <chrono>
#include <csignal>
#include <cstdint>
#include <exception>
#include <iostream>
#include <optional>
#include <stdexcept>
#include <string>
#include <string_view>
#include <system_error>
#include <thread>
#include <unordered_map>
#include <unordered_set>
#include <vector>

namespace {
using Clock = std::chrono::steady_clock;

constexpr size_t kToxAddressHexLength = TOX_ADDRESS_SIZE * 2;
constexpr size_t kToxPublicKeyHexLength = TOX_PUBLIC_KEY_SIZE * 2;

volatile std::sig_atomic_t g_stop_requested = 0;

struct NodeConfig {
    std::string host;
    uint16_t port{0};
    std::string public_key_hex;
};

struct SendEveryConfig {
    std::chrono::seconds interval{};
    std::string message;
};

struct Config {
    std::vector<NodeConfig> bootstrap_nodes;
    std::vector<NodeConfig> tcp_relays;
    bool accept{false};
    std::optional<std::string> add_tox_id_hex;
    std::string request_message{"hi"};
    std::optional<std::string> send_once_message;
    std::optional<SendEveryConfig> send_every;
    std::optional<std::chrono::seconds> delete_after;
    std::optional<std::chrono::seconds> print_friends_every;
    std::optional<std::chrono::seconds> exit_after;
    bool help{false};
};

struct FriendState {
    TOX_CONNECTION connection_status{TOX_CONNECTION_NONE};
    bool send_once_attempted{false};
    std::optional<Clock::time_point> added_at;
    std::optional<Clock::time_point> next_periodic_send;
};

using FriendStates = std::unordered_map<uint32_t, FriendState>;

void HandleSignal(int signal) {
    (void)signal;
    g_stop_requested = 1;
}

std::string ToString(std::string_view value) {
    return std::string(value.data(), value.size());
}

bool IsHexChar(char c) {
    return (c >= '0' && c <= '9') || (c >= 'a' && c <= 'f') ||
           (c >= 'A' && c <= 'F');
}

void ValidateHex(std::string_view option, std::string_view value,
                 size_t expected_length) {
    if (value.size() != expected_length) {
        throw std::invalid_argument(ToString(option) + " expects " +
                                    std::to_string(expected_length) +
                                    " hex characters");
    }

    if (!std::all_of(value.begin(), value.end(), IsHexChar)) {
        throw std::invalid_argument(ToString(option) +
                                    " contains a non-hex character");
    }
}

void ValidateNonEmpty(std::string_view option, std::string_view value) {
    if (value.empty()) {
        throw std::invalid_argument(ToString(option) +
                                    " requires a non-empty value");
    }
}

std::string_view RequireArg(int argc, char* argv[], int& index,
                            std::string_view option,
                            std::string_view value_name) {
    if (index + 1 >= argc) {
        throw std::invalid_argument(ToString(option) + " requires " +
                                    ToString(value_name));
    }

    ++index;
    return argv[index];
}

uint16_t ParsePort(std::string_view option, std::string_view value) {
    ValidateNonEmpty(option, value);

    uint64_t parsed = 0;
    const char* begin = value.data();
    const char* end = value.data() + value.size();
    const auto result = std::from_chars(begin, end, parsed);
    if (result.ec != std::errc{} || result.ptr != end || parsed == 0 ||
        parsed > 65535) {
        throw std::invalid_argument(ToString(option) +
                                    " port must be an integer from 1 to 65535");
    }

    return static_cast<uint16_t>(parsed);
}

std::chrono::seconds ParsePositiveSeconds(std::string_view option,
                                          std::string_view value) {
    ValidateNonEmpty(option, value);

    uint64_t parsed = 0;
    const char* begin = value.data();
    const char* end = value.data() + value.size();
    const auto result = std::from_chars(begin, end, parsed);
    if (result.ec != std::errc{} || result.ptr != end || parsed == 0) {
        throw std::invalid_argument(ToString(option) +
                                    " seconds must be a positive integer");
    }

    const auto max_seconds =
        static_cast<uint64_t>(std::chrono::seconds::max().count());
    if (parsed > max_seconds) {
        throw std::invalid_argument(ToString(option) + " seconds is too large");
    }

    return std::chrono::seconds(parsed);
}

void PrintHelp(std::ostream& out) {
    out << "Usage:\n"
        << "  toxcore_wrap_cli [options]\n\n"
        << "Options:\n"
        << "  --bootstrap <host> <port> <pubKeyHex>    Add a bootstrap node. May be repeated.\n"
        << "  --tcp-relay <host> <port> <pubKeyHex>    Add a TCP relay node. May be repeated.\n"
        << "  --accept                                 Automatically accept friend requests.\n"
        << "  --add <toxIdHex>                         Send a friend request to a Tox ID.\n"
        << "  --request <message>                      Friend request message. Default: hi.\n"
        << "  --send <message>                         Send once after each friend connects.\n"
        << "  --send-every <seconds> <message>         Send periodically while friends are connected.\n"
        << "  --delete-after <seconds>                 Delete added or accepted friends after N seconds.\n"
        << "  --print-friends-every <seconds>          Print local friend numbers every N seconds.\n"
        << "  --exit-after <seconds>                   Exit automatically after N seconds.\n"
        << "  --help                                   Show this help text.\n";
}

Config ParseArgs(int argc, char* argv[]) {
    Config config;

    for (int i = 1; i < argc; ++i) {
        const std::string_view arg = argv[i];

        if (arg == "--help") {
            config.help = true;
            continue;
        }

        if (arg == "--bootstrap" || arg == "--tcp-relay") {
            const std::string_view host = RequireArg(argc, argv, i, arg, "<host>");
            const std::string_view port_arg =
                RequireArg(argc, argv, i, arg, "<port>");
            const std::string_view public_key_hex =
                RequireArg(argc, argv, i, arg, "<pubKeyHex>");

            ValidateNonEmpty(arg, host);
            ValidateHex(arg, public_key_hex, kToxPublicKeyHexLength);

            NodeConfig node{ToString(host), ParsePort(arg, port_arg),
                            ToString(public_key_hex)};
            if (arg == "--bootstrap") {
                config.bootstrap_nodes.push_back(std::move(node));
            } else {
                config.tcp_relays.push_back(std::move(node));
            }
            continue;
        }

        if (arg == "--accept") {
            config.accept = true;
            continue;
        }

        if (arg == "--add") {
            if (config.add_tox_id_hex) {
                throw std::invalid_argument("--add may only be provided once");
            }
            const std::string_view tox_id_hex =
                RequireArg(argc, argv, i, arg, "<toxIdHex>");
            ValidateHex(arg, tox_id_hex, kToxAddressHexLength);
            config.add_tox_id_hex = ToString(tox_id_hex);
            continue;
        }

        if (arg == "--request") {
            const std::string_view message =
                RequireArg(argc, argv, i, arg, "<message>");
            ValidateNonEmpty(arg, message);
            config.request_message = ToString(message);
            continue;
        }

        if (arg == "--send") {
            if (config.send_once_message) {
                throw std::invalid_argument("--send may only be provided once");
            }
            const std::string_view message =
                RequireArg(argc, argv, i, arg, "<message>");
            ValidateNonEmpty(arg, message);
            config.send_once_message = ToString(message);
            continue;
        }

        if (arg == "--send-every") {
            if (config.send_every) {
                throw std::invalid_argument(
                    "--send-every may only be provided once");
            }
            const std::string_view seconds_arg =
                RequireArg(argc, argv, i, arg, "<seconds>");
            const std::string_view message =
                RequireArg(argc, argv, i, arg, "<message>");
            ValidateNonEmpty(arg, message);
            config.send_every =
                SendEveryConfig{ParsePositiveSeconds(arg, seconds_arg),
                                ToString(message)};
            continue;
        }

        if (arg == "--delete-after") {
            if (config.delete_after) {
                throw std::invalid_argument(
                    "--delete-after may only be provided once");
            }
            config.delete_after = ParsePositiveSeconds(
                arg, RequireArg(argc, argv, i, arg, "<seconds>"));
            continue;
        }

        if (arg == "--print-friends-every") {
            if (config.print_friends_every) {
                throw std::invalid_argument(
                    "--print-friends-every may only be provided once");
            }
            config.print_friends_every = ParsePositiveSeconds(
                arg, RequireArg(argc, argv, i, arg, "<seconds>"));
            continue;
        }

        if (arg == "--exit-after") {
            if (config.exit_after) {
                throw std::invalid_argument(
                    "--exit-after may only be provided once");
            }
            config.exit_after = ParsePositiveSeconds(
                arg, RequireArg(argc, argv, i, arg, "<seconds>"));
            continue;
        }

        throw std::invalid_argument("unknown argument: " + ToString(arg));
    }

    return config;
}

std::string ConnectionStatusName(TOX_CONNECTION status) {
    switch (status) {
        case TOX_CONNECTION_NONE:
            return "none";
        case TOX_CONNECTION_TCP:
            return "tcp";
        case TOX_CONNECTION_UDP:
            return "udp";
    }

    return "unknown(" + std::to_string(static_cast<int>(status)) + ")";
}

void UpdateConnectionState(FriendState& state, TOX_CONNECTION status,
                           Clock::time_point now) {
    const bool was_connected =
        state.connection_status != TOX_CONNECTION_NONE;
    const bool is_connected = status != TOX_CONNECTION_NONE;

    state.connection_status = status;

    if (is_connected && !was_connected) {
        state.next_periodic_send = now;
    } else if (!is_connected) {
        state.next_periodic_send.reset();
    }
}

void MarkFriendAdded(FriendStates& friend_states, uint32_t friend_number,
                     Clock::time_point now) {
    FriendState& state = friend_states[friend_number];
    state.added_at = now;
}

bool TrySendFriendMessage(ToxCore::ToxCoreWrapper& tox, uint32_t friend_number,
                          const std::string& message,
                          std::string_view reason) {
    try {
        const uint32_t message_id = tox.SendFriendMessage(friend_number, message);
        std::cout << "[send] reason=" << reason << " friend=" << friend_number
                  << " message_id=" << message_id << " text=" << message
                  << std::endl;
        return true;
    } catch (const std::exception& ex) {
        std::cerr << "[error] send failed friend=" << friend_number
                  << " reason=" << reason << ": " << ex.what() << std::endl;
        return false;
    }
}

void SyncFriendStates(ToxCore::ToxCoreWrapper& tox,
                      FriendStates& friend_states, Clock::time_point now) {
    const std::vector<uint32_t> friend_list = tox.GetFriendList();
    std::unordered_set<uint32_t> existing_friends;
    existing_friends.reserve(friend_list.size());

    for (uint32_t friend_number : friend_list) {
        existing_friends.insert(friend_number);
        FriendState& state = friend_states[friend_number];
        try {
            UpdateConnectionState(
                state, tox.GetFriendConnectionStatus(friend_number), now);
        } catch (const std::exception& ex) {
            std::cerr << "[error] failed to query friend=" << friend_number
                      << " connection status: " << ex.what() << std::endl;
        }
    }

    for (auto it = friend_states.begin(); it != friend_states.end();) {
        if (!existing_friends.contains(it->first)) {
            it = friend_states.erase(it);
        } else {
            ++it;
        }
    }
}

void ApplyNetworkNodes(ToxCore::ToxCoreWrapper& tox, const Config& config) {
    for (const NodeConfig& node : config.bootstrap_nodes) {
        try {
            tox.Bootstrap(node.host, node.port, node.public_key_hex);
            std::cout << "[bootstrap] ok host=" << node.host
                      << " port=" << node.port << std::endl;
        } catch (const std::exception& ex) {
            std::cerr << "[bootstrap] failed host=" << node.host
                      << " port=" << node.port << ": " << ex.what()
                      << std::endl;
        }
    }

    for (const NodeConfig& node : config.tcp_relays) {
        try {
            tox.AddTcpRelay(node.host, node.port, node.public_key_hex);
            std::cout << "[tcp-relay] ok host=" << node.host
                      << " port=" << node.port << std::endl;
        } catch (const std::exception& ex) {
            std::cerr << "[tcp-relay] failed host=" << node.host
                      << " port=" << node.port << ": " << ex.what()
                      << std::endl;
        }
    }
}

void PrintFriendList(ToxCore::ToxCoreWrapper& tox) {
    try {
        const std::vector<uint32_t> friend_list = tox.GetFriendList();
        std::cout << "[friends]";
        if (friend_list.empty()) {
            std::cout << " (empty)";
        } else {
            for (uint32_t friend_number : friend_list) {
                std::cout << ' ' << friend_number;
            }
        }
        std::cout << std::endl;
    } catch (const std::exception& ex) {
        std::cerr << "[error] failed to print friend list: " << ex.what()
                  << std::endl;
    }
}

void ProcessDeletes(const Config& config, ToxCore::ToxCoreWrapper& tox,
                    FriendStates& friend_states, Clock::time_point now) {
    if (!config.delete_after) {
        return;
    }

    std::vector<uint32_t> due_friends;
    for (const auto& [friend_number, state] : friend_states) {
        if (state.added_at &&
            now >= *state.added_at + *config.delete_after) {
            due_friends.push_back(friend_number);
        }
    }

    for (uint32_t friend_number : due_friends) {
        try {
            tox.DeleteFriend(friend_number);
            friend_states.erase(friend_number);
            std::cout << "[delete] friend=" << friend_number << std::endl;
        } catch (const std::exception& ex) {
            auto it = friend_states.find(friend_number);
            if (it != friend_states.end()) {
                it->second.added_at.reset();
            }
            std::cerr << "[error] delete failed friend=" << friend_number
                      << ": " << ex.what() << std::endl;
        }
    }
}

void ProcessSendOnce(const Config& config, ToxCore::ToxCoreWrapper& tox,
                     FriendStates& friend_states) {
    if (!config.send_once_message) {
        return;
    }

    for (auto& [friend_number, state] : friend_states) {
        if (state.connection_status == TOX_CONNECTION_NONE ||
            state.send_once_attempted) {
            continue;
        }

        state.send_once_attempted = true;
        TrySendFriendMessage(tox, friend_number, *config.send_once_message,
                             "send-once");
    }
}

void ProcessPeriodicSends(const Config& config, ToxCore::ToxCoreWrapper& tox,
                          FriendStates& friend_states, Clock::time_point now) {
    if (!config.send_every) {
        return;
    }

    for (auto& [friend_number, state] : friend_states) {
        if (state.connection_status == TOX_CONNECTION_NONE) {
            continue;
        }

        if (!state.next_periodic_send) {
            state.next_periodic_send = now;
        }

        if (now < *state.next_periodic_send) {
            continue;
        }

        state.next_periodic_send = now + config.send_every->interval;
        TrySendFriendMessage(tox, friend_number, config.send_every->message,
                             "send-every");
    }
}

std::chrono::milliseconds ComputeSleepDelay(
    const ToxCore::ToxCoreWrapper& tox, const Config& config,
    const FriendStates& friend_states, Clock::time_point now,
    std::optional<Clock::time_point> next_print_friends,
    std::optional<Clock::time_point> exit_deadline) {
    uint32_t interval_ms = tox.IterationIntervalMs();
    if (interval_ms == 0) {
        interval_ms = 50;
    }
    interval_ms = std::clamp(interval_ms, 10u, 1000u);

    std::chrono::milliseconds delay(interval_ms);

    auto consider_deadline = [&](Clock::time_point deadline) {
        if (deadline <= now) {
            delay = std::chrono::milliseconds(1);
            return;
        }

        const auto remaining =
            std::chrono::duration_cast<std::chrono::milliseconds>(deadline -
                                                                  now);
        delay = std::min(delay, remaining);
    };

    if (next_print_friends) {
        consider_deadline(*next_print_friends);
    }
    if (exit_deadline) {
        consider_deadline(*exit_deadline);
    }

    if (config.delete_after) {
        for (const auto& [friend_number, state] : friend_states) {
            (void)friend_number;
            if (state.added_at) {
                consider_deadline(*state.added_at + *config.delete_after);
            }
        }
    }

    if (config.send_every) {
        for (const auto& [friend_number, state] : friend_states) {
            (void)friend_number;
            if (state.connection_status != TOX_CONNECTION_NONE &&
                state.next_periodic_send) {
                consider_deadline(*state.next_periodic_send);
            }
        }
    }

    return std::max(delay, std::chrono::milliseconds(1));
}

int Run(const Config& config) {
    ToxCore::ToxCoreWrapper tox;
    FriendStates friend_states;

    std::cout << "[self] tox_id=" << tox.GetSelfAddressHex() << std::endl;

    tox.SetOnFriendRequest(
        [&](const std::string& public_key_hex, const std::string& message) {
            std::cout << "[event] friend_request public_key=" << public_key_hex
                      << " message=" << message << std::endl;

            if (!config.accept) {
                std::cout << "[event] friend_request ignored" << std::endl;
                return;
            }

            try {
                const uint32_t friend_number =
                    tox.AddFriendNoRequest(public_key_hex);
                MarkFriendAdded(friend_states, friend_number, Clock::now());
                std::cout << "[accept] friend=" << friend_number << std::endl;
            } catch (const std::exception& ex) {
                std::cerr << "[error] accept failed public_key="
                          << public_key_hex << ": " << ex.what() << std::endl;
            }
        });

    tox.SetOnFriendConnectionStatus(
        [&](uint32_t friend_number, TOX_CONNECTION status) {
            FriendState& state = friend_states[friend_number];
            UpdateConnectionState(state, status, Clock::now());
            std::cout << "[event] friend_connection friend=" << friend_number
                      << " status=" << ConnectionStatusName(status)
                      << std::endl;
        });

    tox.SetOnFriendMessage(
        [](uint32_t friend_number, TOX_MESSAGE_TYPE type,
           const std::string& message) {
            std::cout << "[event] friend_message friend=" << friend_number
                      << " type=" << static_cast<int>(type)
                      << " message=" << message << std::endl;
        });

    ApplyNetworkNodes(tox, config);

    if (config.add_tox_id_hex) {
        try {
            const uint32_t friend_number =
                tox.AddFriend(*config.add_tox_id_hex, config.request_message);
            MarkFriendAdded(friend_states, friend_number, Clock::now());
            std::cout << "[add] friend=" << friend_number << std::endl;
        } catch (const std::exception& ex) {
            std::cerr << "[error] add failed: " << ex.what() << std::endl;
            return 1;
        }
    }

    const Clock::time_point started_at = Clock::now();
    std::optional<Clock::time_point> next_print_friends;
    if (config.print_friends_every) {
        next_print_friends = started_at;
    }

    std::optional<Clock::time_point> exit_deadline;
    if (config.exit_after) {
        exit_deadline = started_at + *config.exit_after;
    }

    TOX_CONNECTION last_self_status = tox.GetSelfConnectionStatus();
    std::cout << "[self] connection=" << ConnectionStatusName(last_self_status)
              << std::endl;

    while (g_stop_requested == 0) {
        tox.Iterate();

        const Clock::time_point now = Clock::now();
        if (exit_deadline && now >= *exit_deadline) {
            std::cout << "[exit] exit-after reached" << std::endl;
            break;
        }

        const TOX_CONNECTION self_status = tox.GetSelfConnectionStatus();
        if (self_status != last_self_status) {
            last_self_status = self_status;
            std::cout << "[self] connection="
                      << ConnectionStatusName(self_status) << std::endl;
        }

        SyncFriendStates(tox, friend_states, now);
        ProcessDeletes(config, tox, friend_states, now);
        ProcessSendOnce(config, tox, friend_states);
        ProcessPeriodicSends(config, tox, friend_states, now);

        if (next_print_friends && now >= *next_print_friends) {
            PrintFriendList(tox);
            next_print_friends = now + *config.print_friends_every;
        }

        const auto sleep_delay = ComputeSleepDelay(
            tox, config, friend_states, Clock::now(), next_print_friends,
            exit_deadline);
        std::this_thread::sleep_for(sleep_delay);
    }

    if (g_stop_requested != 0) {
        std::cout << "[exit] signal received" << std::endl;
    }

    return 0;
}

}  // namespace

int main(int argc, char* argv[]) {
    std::signal(SIGINT, HandleSignal);
#ifdef SIGTERM
    std::signal(SIGTERM, HandleSignal);
#endif

    try {
        const Config config = ParseArgs(argc, argv);
        if (config.help) {
            PrintHelp(std::cout);
            return 0;
        }

        return Run(config);
    } catch (const std::invalid_argument& ex) {
        std::cerr << "error: " << ex.what() << "\n\n";
        PrintHelp(std::cerr);
        return 2;
    } catch (const std::exception& ex) {
        std::cerr << "fatal: " << ex.what() << std::endl;
        return 1;
    }
}
