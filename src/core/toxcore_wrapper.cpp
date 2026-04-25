#include "core/toxcore_wrapper.h"

#include <algorithm>
#include <array>
#include <fstream>
#include <iostream>
#include <limits>
#include <stdexcept>
#include <string_view>
#include <utility>

namespace ToxCore {
namespace {
/**
 * @brief Convert a single hexadecimal character to its 4-bit value (nibble)
 * @param c Hexadecimal character ('0'-'9', 'a'-'f', 'A'-'F')
 * @return 4-bit value corresponding to the hexadecimal character
 * @throws std::invalid_argument if the character is not a valid hexadecimal
 * digit
 */
uint8_t HexNibble(char c) {
    if (c >= '0' && c <= '9') {
        return static_cast<uint8_t>(c - '0');
    }
    if (c >= 'a' && c <= 'f') {
        return static_cast<uint8_t>(10 + (c - 'a'));
    }
    if (c >= 'A' && c <= 'F') {
        return static_cast<uint8_t>(10 + (c - 'A'));
    }

    throw std::invalid_argument("hex string contains non-hex character");
}

/**
 * @brief Decode a fixed-size hexadecimal string into a byte array.
 * @tparam N Number of bytes to decode.
 * @param hex Hexadecimal string to decode (must be exactly N*2 characters).
 * @return std::array<uint8_t, N> containing the decoded bytes.
 * @throws std::invalid_argument if the string length is incorrect or contains
 * invalid characters.
 */
template <size_t N>
std::array<uint8_t, N> DecodeHexFixed(std::string_view hex) {
    if (hex.size() != N * 2) {
        throw std::invalid_argument("hex string has invalid length");
    }

    std::array<uint8_t, N> out{};
    for (size_t i = 0; i < N; ++i) {
        const uint8_t hi = HexNibble(hex[i * 2]);
        const uint8_t lo = HexNibble(hex[i * 2 + 1]);
        out[i] = static_cast<uint8_t>((hi << 4) | lo);
    }
    return out;
}

/**
 * @brief Encodes a byte buffer as an uppercase hexadecimal string.
 * @param data Pointer to the input byte buffer; the first 'length' bytes are
 * encoded.
 * @param length Number of bytes to encode from data.
 * @return A std::string containing uppercase hexadecimal characters (two
 * characters per byte). Returns an empty string if length is zero.
 */
std::string EncodeHexUpper(const uint8_t* data, size_t length) {
    static constexpr char hex[] = "0123456789ABCDEF";

    std::string result;
    result.resize(length * 2);

    for (size_t i = 0; i < length; ++i) {
        result[2 * i] = hex[data[i] >> 4];
        result[2 * i + 1] = hex[data[i] & 0x0F];
    }

    return result;
}

/**
 * @brief Decode a hexadecimal string into a byte vector.
 * @param hex Hexadecimal string to decode.
 * @param out Vector to store the decoded bytes.
 * @return True if decoding was successful, false otherwise.
 */
bool DecodeHex(std::string_view hex, std::vector<uint8_t>& out) {
    if (hex.size() % 2 != 0) {
        return false;  // Hex string must have an even length
    }
    out.clear();
    out.reserve(hex.size() / 2);
    for (size_t i = 0; i < hex.size(); i += 2) {
        try {
            uint8_t byte = (HexNibble(hex[i]) << 4) | HexNibble(hex[i + 1]);
            out.push_back(byte);
        } catch (const std::invalid_argument&) {
            return false;  // Invalid hex character encountered
        }
    }
    return true;
}
}  // namespace

// ==================== Lifecycle Management ====================

ToxCoreWrapper::ToxCoreWrapper() {
    Tox_Options* options = tox_options_new(nullptr);

    if (!options) {
        throw std::runtime_error("tox_options_new failed");
    }

    TOX_ERR_NEW new_err;
    tox_ = tox_new(options, &new_err);

    tox_options_free(options);

    if (!tox_ || new_err != TOX_ERR_NEW_OK) {
        throw std::runtime_error("tox_new failed: " + std::to_string(new_err));
    }

    InitAv_();
}

ToxCoreWrapper::ToxCoreWrapper(const std::vector<uint8_t>& savedata) {
    if (savedata.empty()) {
        throw std::invalid_argument("Savedata cannot be empty");
    }

    Tox_Options* options = tox_options_new(nullptr);
    if (!options) {
        throw std::runtime_error("tox_options_new failed");
    }

    options->savedata_type = TOX_SAVEDATA_TYPE_TOX_SAVE;
    options->savedata_data = savedata.data();
    options->savedata_length = savedata.size();

    TOX_ERR_NEW new_err;
    tox_ = tox_new(options, &new_err);
    tox_options_free(options);
    if (!tox_ || new_err != TOX_ERR_NEW_OK) {
        throw std::runtime_error("tox_new (with savedata) failed: " +
                                 std::to_string(new_err));
    }

    InitAv_();
}

ToxCoreWrapper::ToxCoreWrapper(ToxCoreWrapper&& other) noexcept
    : onFriendRequest_(std::move(other.onFriendRequest_)),
      onFriendConnectionStatus_(std::move(other.onFriendConnectionStatus_)),
      onFriendMessage_(std::move(other.onFriendMessage_)),
      onFileRecv_(std::move(other.onFileRecv_)),
      onFileRecvControl_(std::move(other.onFileRecvControl_)),
      onFileChunkRequest_(std::move(other.onFileChunkRequest_)),
      onFileRecvChunk_(std::move(other.onFileRecvChunk_)),
      onConferenceInvite_(std::move(other.onConferenceInvite_)),
      onConferenceConnected_(std::move(other.onConferenceConnected_)),
      onConferenceMessage_(std::move(other.onConferenceMessage_)),
      onConferenceTitle_(std::move(other.onConferenceTitle_)),
      onConferencePeerName_(std::move(other.onConferencePeerName_)),
      onConferencePeerListChanged_(
          std::move(other.onConferencePeerListChanged_)),
      onCall_(std::move(other.onCall_)),
      onCallState_(std::move(other.onCallState_)),
      onAudioFrame_(std::move(other.onAudioFrame_)),
      onVideoFrame_(std::move(other.onVideoFrame_)),
      tox_(other.tox_),
      av_(other.av_) {
    other.tox_ = nullptr;
    other.av_ = nullptr;
    RefreshCallbacks_();
}

ToxCoreWrapper& ToxCoreWrapper::operator=(ToxCoreWrapper&& other) noexcept {
    if (this == &other) {
        return *this;
    }

    if (av_) {
        toxav_kill(av_);
        av_ = nullptr;
    }

    if (tox_) {
        tox_kill(tox_);
        tox_ = nullptr;
    }

    onFriendRequest_ = std::move(other.onFriendRequest_);
    onFriendConnectionStatus_ = std::move(other.onFriendConnectionStatus_);
    onFriendMessage_ = std::move(other.onFriendMessage_);
    onFileRecv_ = std::move(other.onFileRecv_);
    onFileRecvControl_ = std::move(other.onFileRecvControl_);
    onFileChunkRequest_ = std::move(other.onFileChunkRequest_);
    onFileRecvChunk_ = std::move(other.onFileRecvChunk_);
    onConferenceInvite_ = std::move(other.onConferenceInvite_);
    onConferenceConnected_ = std::move(other.onConferenceConnected_);
    onConferenceMessage_ = std::move(other.onConferenceMessage_);
    onConferenceTitle_ = std::move(other.onConferenceTitle_);
    onConferencePeerName_ = std::move(other.onConferencePeerName_);
    onConferencePeerListChanged_ =
        std::move(other.onConferencePeerListChanged_);
    onCall_ = std::move(other.onCall_);
    onCallState_ = std::move(other.onCallState_);
    onAudioFrame_ = std::move(other.onAudioFrame_);
    onVideoFrame_ = std::move(other.onVideoFrame_);

    tox_ = other.tox_;
    av_ = other.av_;
    other.tox_ = nullptr;
    other.av_ = nullptr;

    RefreshCallbacks_();

    return *this;
}

ToxCoreWrapper::~ToxCoreWrapper() {
    if (av_) {
        toxav_kill(av_);
        av_ = nullptr;
    }
    if (tox_) {
        tox_kill(tox_);
        tox_ = nullptr;
    }
}

// ==================== Core Loop Management ====================

void ToxCoreWrapper::Iterate() {
    if (!tox_) {
        return;
    }
    tox_iterate(tox_, this);
    if (av_) {
        toxav_iterate(av_);
    }
}

uint32_t ToxCoreWrapper::IterationIntervalMs() const {
    if (!tox_) {
        return 0;
    }

    const uint32_t core_ms = tox_iteration_interval(tox_);
    const uint32_t av_ms = av_ ? toxav_iteration_interval(av_) : core_ms;

    return std::min(core_ms, av_ms);
}

// ==================== Self Information Management ====================

std::string ToxCoreWrapper::GetSelfAddressHex() const {
    EnsureTox_();

    std::array<uint8_t, TOX_ADDRESS_SIZE> address{};
    tox_self_get_address(tox_, address.data());

    return EncodeHexUpper(address.data(), address.size());
}

std::vector<uint8_t> ToxCoreWrapper::GetSavedata() const {
    EnsureTox_();

    const size_t data_size = tox_get_savedata_size(tox_);
    if (data_size == 0) {
        return {};
    }

    std::vector<uint8_t> savedata(data_size);
    tox_get_savedata(tox_, savedata.data());
    return savedata;
}

void ToxCoreWrapper::SetSelfNospam(uint32_t nospam) {
    EnsureTox_();

    tox_self_set_nospam(tox_, nospam);
}

uint32_t ToxCoreWrapper::GetSelfNospam() const {
    EnsureTox_();
    return tox_self_get_nospam(tox_);
}

void ToxCoreWrapper::SetSelfName(const std::string& name) {
    EnsureTox_();

    if (name.size() > TOX_MAX_NAME_LENGTH) {
        throw std::invalid_argument("Name exceeds maximum length");
    }

    TOX_ERR_SET_INFO err = TOX_ERR_SET_INFO_OK;
    const uint8_t* name_data =
        name.empty() ? nullptr : reinterpret_cast<const uint8_t*>(name.data());
    const bool ok = tox_self_set_name(tox_, name_data, name.size(), &err);
    if (!ok || err != TOX_ERR_SET_INFO_OK) {
        throw std::runtime_error("tox_self_set_name failed, err=" +
                                 std::to_string(static_cast<int>(err)));
    }
}

std::string ToxCoreWrapper::GetSelfName() const {
    EnsureTox_();

    const size_t name_size = tox_self_get_name_size(tox_);
    if (name_size == 0) {
        return {};
    }

    std::string name(name_size, '\0');
    tox_self_get_name(tox_, reinterpret_cast<uint8_t*>(name.data()));
    return name;
}

void ToxCoreWrapper::SetSelfStatusMessage(const std::string& status_message) {
    EnsureTox_();

    if (status_message.size() > TOX_MAX_STATUS_MESSAGE_LENGTH) {
        throw std::invalid_argument("Status message exceeds maximum length");
    }

    TOX_ERR_SET_INFO err = TOX_ERR_SET_INFO_OK;
    const uint8_t* message_data =
        status_message.empty()
            ? nullptr
            : reinterpret_cast<const uint8_t*>(status_message.data());

    const bool ok = tox_self_set_status_message(tox_, message_data,
                                                status_message.size(), &err);
    if (!ok || err != TOX_ERR_SET_INFO_OK) {
        throw std::runtime_error("tox_self_set_status_message failed, err=" +
                                 std::to_string(static_cast<int>(err)));
    }
}

std::string ToxCoreWrapper::GetSelfStatusMessage() const {
    EnsureTox_();

    const size_t message_size = tox_self_get_status_message_size(tox_);
    if (message_size == 0) {
        return {};
    }

    std::string status_message(message_size, '\0');
    tox_self_get_status_message(
        tox_, reinterpret_cast<uint8_t*>(status_message.data()));
    return status_message;
}

void ToxCoreWrapper::SetSelfStatus(Tox_User_Status status) {
    EnsureTox_();

    tox_self_set_status(tox_, status);
}

Tox_User_Status ToxCoreWrapper::GetSelfStatus() const {
    EnsureTox_();

    return tox_self_get_status(tox_);
}

// ==================== Friend Management ====================

uint32_t ToxCoreWrapper::AddFriend(const std::string& tox_id_hex,
                                   const std::string& message) {
    EnsureTox_();

    if (message.empty()) {
        throw std::invalid_argument("Friend request message cannot be empty");
    }

    if (message.size() > TOX_MAX_FRIEND_REQUEST_LENGTH) {
        throw std::invalid_argument(
            "Friend request message exceeds maximum length");
    }

    const auto address = DecodeHexFixed<TOX_ADDRESS_SIZE>(tox_id_hex);

    TOX_ERR_FRIEND_ADD err = TOX_ERR_FRIEND_ADD_OK;
    const uint32_t friend_num = tox_friend_add(
        tox_, address.data(), reinterpret_cast<const uint8_t*>(message.data()),
        message.size(), &err);

    if (err != TOX_ERR_FRIEND_ADD_OK) {
        throw std::runtime_error("tox_friend_add failed, err=" +
                                 std::to_string(static_cast<int>(err)));
    }

    return friend_num;
}

uint32_t ToxCoreWrapper::AddFriendNoRequest(const std::string& public_key_hex) {
    EnsureTox_();

    const auto public_key = DecodeHexFixed<TOX_PUBLIC_KEY_SIZE>(public_key_hex);

    TOX_ERR_FRIEND_ADD err = TOX_ERR_FRIEND_ADD_OK;
    const uint32_t friend_num =
        tox_friend_add_norequest(tox_, public_key.data(), &err);

    if (err != TOX_ERR_FRIEND_ADD_OK) {
        throw std::runtime_error("tox_friend_add_norequest failed, err=" +
                                 std::to_string(static_cast<int>(err)));
    }

    return friend_num;
}

void ToxCoreWrapper::DeleteFriend(uint32_t friend_number) {
    EnsureTox_();

    TOX_ERR_FRIEND_DELETE err{};
    const bool ok = tox_friend_delete(tox_, friend_number, &err);
    if (!ok) {
        throw std::runtime_error("tox_friend_delete failed, err=" +
                                 std::to_string(static_cast<int>(err)));
    }
}

std::vector<uint32_t> ToxCoreWrapper::GetFriendList() const {
    EnsureTox_();

    const size_t friend_count = tox_self_get_friend_list_size(tox_);
    std::vector<uint32_t> friend_list(friend_count);

    if (friend_count > 0) {
        tox_self_get_friend_list(tox_, friend_list.data());
    }

    return friend_list;
}

bool ToxCoreWrapper::FriendExists(uint32_t friend_number) const {
    EnsureTox_();

    return tox_friend_exists(tox_, friend_number);
}

std::string ToxCoreWrapper::GetFriendPublicKeyHex(
    uint32_t friend_number) const {
    EnsureTox_();

    std::array<uint8_t, TOX_PUBLIC_KEY_SIZE> public_key{};
    TOX_ERR_FRIEND_GET_PUBLIC_KEY err = TOX_ERR_FRIEND_GET_PUBLIC_KEY_OK;

    const bool ok =
        tox_friend_get_public_key(tox_, friend_number, public_key.data(), &err);

    if (!ok || err != TOX_ERR_FRIEND_GET_PUBLIC_KEY_OK) {
        throw std::runtime_error("tox_friend_get_public_key failed, err=" +
                                 std::to_string(static_cast<int>(err)));
    }

    return EncodeHexUpper(public_key.data(), public_key.size());
}

std::string ToxCoreWrapper::GetFriendName(uint32_t friend_number) const {
    if (!tox_) {
        return "";
    }

    TOX_ERR_FRIEND_QUERY err = TOX_ERR_FRIEND_QUERY_OK;
    const size_t name_size =
        tox_friend_get_name_size(tox_, friend_number, &err);
    if (err != TOX_ERR_FRIEND_QUERY_OK) {
        throw std::runtime_error("tox_friend_get_name_size failed, err=" +
                                 std::to_string(static_cast<int>(err)));
    }
    if (name_size == 0) {
        return "";
    }

    std::vector<uint8_t> name_data(name_size);
    if (!tox_friend_get_name(tox_, friend_number, name_data.data(), &err) ||
        err != TOX_ERR_FRIEND_QUERY_OK) {
        return "";
    }
    return std::string(name_data.begin(), name_data.end());
}

TOX_CONNECTION ToxCoreWrapper::GetFriendConnectionStatus(
    uint32_t friend_number) const {
    EnsureTox_();

    TOX_ERR_FRIEND_QUERY err = TOX_ERR_FRIEND_QUERY_OK;
    const TOX_CONNECTION status =
        tox_friend_get_connection_status(tox_, friend_number, &err);

    if (err != TOX_ERR_FRIEND_QUERY_OK) {
        throw std::runtime_error(
            "tox_friend_get_connection_status failed, err=" +
            std::to_string(static_cast<int>(err)));
    }

    return status;
}

void ToxCoreWrapper::SetOnFriendRequest(FriendRequestCb cb) {
    onFriendRequest_ = std::move(cb);
    RefreshCallbacks_();
}

void ToxCoreWrapper::SetOnFriendConnectionStatus(FriendConnectionStatusCb cb) {
    onFriendConnectionStatus_ = std::move(cb);
    RefreshCallbacks_();
}

// ==================== Message Management ====================

uint32_t ToxCoreWrapper::SendFriendMessage(uint32_t friend_number,
                                           const std::string& message,
                                           TOX_MESSAGE_TYPE type) {
    EnsureTox_();

    if (message.empty()) {
        throw std::invalid_argument("Message cannot be empty");
    }

    if (message.size() > TOX_MAX_MESSAGE_LENGTH) {
        throw std::invalid_argument("Message exceeds maximum length");
    }

    TOX_ERR_FRIEND_SEND_MESSAGE err = TOX_ERR_FRIEND_SEND_MESSAGE_OK;

    const uint32_t message_id = tox_friend_send_message(
        tox_, friend_number, type,
        reinterpret_cast<const uint8_t*>(message.data()), message.size(), &err);

    if (err != TOX_ERR_FRIEND_SEND_MESSAGE_OK) {
        throw std::runtime_error("tox_friend_send_message failed, err=" +
                                 std::to_string(static_cast<int>(err)));
    }

    return message_id;
}

void ToxCoreWrapper::SetOnFriendMessage(FriendMessageCb cb) {
    onFriendMessage_ = std::move(cb);
    RefreshCallbacks_();
}

// ==================== Network Management ====================

void ToxCoreWrapper::Bootstrap(const std::string& address, uint16_t port,
                               const std::string& public_key_hex) {
    EnsureTox_();

    const auto public_key = DecodeHexFixed<TOX_PUBLIC_KEY_SIZE>(public_key_hex);

    TOX_ERR_BOOTSTRAP err = TOX_ERR_BOOTSTRAP_OK;
    const bool ok =
        tox_bootstrap(tox_, address.c_str(), port, public_key.data(), &err);
    if (!ok || err != TOX_ERR_BOOTSTRAP_OK) {
        throw std::runtime_error("tox_bootstrap failed, err=" +
                                 std::to_string(static_cast<int>(err)));
    }
}

void ToxCoreWrapper::AddTcpRelay(const std::string& address, uint16_t port,
                                 const std::string& public_key_hex) {
    EnsureTox_();

    TOX_ERR_BOOTSTRAP err = TOX_ERR_BOOTSTRAP_OK;

    const auto public_key = DecodeHexFixed<TOX_PUBLIC_KEY_SIZE>(public_key_hex);
    const bool ok =
        tox_add_tcp_relay(tox_, address.c_str(), port, public_key.data(), &err);
    if (!ok || err != TOX_ERR_BOOTSTRAP_OK) {
        throw std::runtime_error("tox_add_tcp_relay failed, err=" +
                                 std::to_string(static_cast<int>(err)));
    }
}

TOX_CONNECTION ToxCoreWrapper::GetSelfConnectionStatus() const {
    EnsureTox_();
    return tox_self_get_connection_status(tox_);
}

// ==================== File Transfer Management ====================

uint32_t ToxCoreWrapper::SendFile(uint32_t friend_number,
                                  const std::filesystem::path& file_path,
                                  const std::string& file_name_utf8) {
    EnsureTox_();

    std::ifstream file(file_path, std::ios::binary | std::ios::ate);
    if (!file.is_open()) {
        std::cerr << "Failed to open file: " << file_path.string() << std::endl;
        return std::numeric_limits<uint32_t>::max();
    }

    uint64_t file_size = static_cast<uint64_t>(file.tellg());
    file.close();

    TOX_ERR_FILE_SEND err = TOX_ERR_FILE_SEND_OK;

    uint32_t file_number = tox_file_send(
        tox_, friend_number, TOX_FILE_KIND_DATA, file_size, nullptr,
        reinterpret_cast<const uint8_t*>(file_name_utf8.data()),
        file_name_utf8.size(), &err);

    if (err != TOX_ERR_FILE_SEND_OK) {
        std::cerr << "tox_file_send failed with error: " << err << std::endl;
        return std::numeric_limits<uint32_t>::max();
    }

    return file_number;
}

bool ToxCoreWrapper::ControlFileTransfer(uint32_t friend_number,
                                         uint32_t file_number,
                                         TOX_FILE_CONTROL control) {
    EnsureTox_();

    TOX_ERR_FILE_CONTROL err;

    bool ok = tox_file_control(tox_, friend_number, file_number, control, &err);

    if (!ok || err != TOX_ERR_FILE_CONTROL_OK) {
        std::cerr << "tox_file_control failed with error: " << err << std::endl;
        return false;
    }
    return true;
}

bool ToxCoreWrapper::SendFileChunk(uint32_t friend_number, uint32_t file_number,
                                   uint64_t position,
                                   const std::vector<uint8_t>& data) {
    EnsureTox_();

    TOX_ERR_FILE_SEND_CHUNK err = TOX_ERR_FILE_SEND_CHUNK_OK;

    const bool ok =
        tox_file_send_chunk(tox_, friend_number, file_number, position,
                            data.data(), data.size(), &err);

    if (!ok || err != TOX_ERR_FILE_SEND_CHUNK_OK) {
        std::cerr << "tox_file_send_chunk failed with error: " << err
                  << std::endl;
        return false;
    }

    return true;
}

void ToxCoreWrapper::SetOnFileRecv(FileRecvCb cb) {
    onFileRecv_ = std::move(cb);
    RefreshCallbacks_();
}

void ToxCoreWrapper::SetOnFileRecvControl(FileRecvControlCb cb) {
    onFileRecvControl_ = std::move(cb);
    RefreshCallbacks_();
}

void ToxCoreWrapper::SetOnFileChunkRequest(FileChunkRequestCb cb) {
    onFileChunkRequest_ = std::move(cb);
    RefreshCallbacks_();
}

void ToxCoreWrapper::SetOnFileRecvChunk(FileRecvChunkCb cb) {
    onFileRecvChunk_ = std::move(cb);
    RefreshCallbacks_();
}

// ==================== Conference Management ====================

uint32_t ToxCoreWrapper::CreateConference() {
    EnsureTox_();

    TOX_ERR_CONFERENCE_NEW err = TOX_ERR_CONFERENCE_NEW_OK;

    uint32_t conference_number = tox_conference_new(tox_, &err);
    if (err != TOX_ERR_CONFERENCE_NEW_OK) {
        std::cerr << "tox_conference_new failed with error: " << err
                  << std::endl;
        return std::numeric_limits<uint32_t>::max();
    }
    return conference_number;
}

bool ToxCoreWrapper::DeleteConference(uint32_t conference_number) {
    EnsureTox_();
    TOX_ERR_CONFERENCE_DELETE err = TOX_ERR_CONFERENCE_DELETE_OK;

    const bool ok = tox_conference_delete(tox_, conference_number, &err);

    return ok && (err == TOX_ERR_CONFERENCE_DELETE_OK);
}

bool ToxCoreWrapper::InviteFriendToConference(uint32_t conference_number,
                                              uint32_t friend_number) {
    EnsureTox_();

    TOX_ERR_CONFERENCE_INVITE err = TOX_ERR_CONFERENCE_INVITE_OK;

    const bool ok =
        tox_conference_invite(tox_, friend_number, conference_number, &err);
    return ok && (err == TOX_ERR_CONFERENCE_INVITE_OK);
}

bool ToxCoreWrapper::JoinConference(uint32_t friend_number,
                                    const std::vector<uint8_t>& cookie) {
    EnsureTox_();

    TOX_ERR_CONFERENCE_JOIN err = TOX_ERR_CONFERENCE_JOIN_OK;
    uint32_t conference_number = tox_conference_join(
        tox_, friend_number, cookie.data(), cookie.size(), &err);
    return (err == TOX_ERR_CONFERENCE_JOIN_OK) &&
           (conference_number != std::numeric_limits<uint32_t>::max());
}

// ================= Conference Information Retrieval ===============

std::vector<uint8_t> ToxCoreWrapper::GetConferenceId(
    uint32_t conference_number) const {
    if (!tox_) {
        return {};
    }

    std::vector<uint8_t> id(TOX_CONFERENCE_ID_SIZE);
    if (!tox_conference_get_id(tox_, conference_number, id.data())) {
        return {};
    }

    return id;
}

std::optional<uint32_t> ToxCoreWrapper::GetConferenceByIdHex(
    const std::string& id_hex) const {
    if (!tox_ || id_hex.size() != TOX_CONFERENCE_ID_SIZE * 2) {
        return std::nullopt;
    }

    std::vector<uint8_t> id;
    if (!DecodeHex(id_hex, id) || id.size() != TOX_CONFERENCE_ID_SIZE) {
        return std::nullopt;
    }

    TOX_ERR_CONFERENCE_BY_ID err = TOX_ERR_CONFERENCE_BY_ID_OK;
    uint32_t conference_number = tox_conference_by_id(tox_, id.data(), &err);
    if (err != TOX_ERR_CONFERENCE_BY_ID_OK) {
        return std::nullopt;
    }

    return conference_number;
}

std::vector<uint32_t> ToxCoreWrapper::GetConferenceList() const {
    if (!tox_) {
        return {};
    }

    size_t count = tox_conference_get_chatlist_size(tox_);

    if (count == 0) {
        return {};
    }

    std::vector<uint32_t> conference_list(count);
    tox_conference_get_chatlist(tox_, conference_list.data());

    return conference_list;
}

// ============== Conference Information Management ===============

bool ToxCoreWrapper::SetConferenceTitle(uint32_t conference_number,
                                        const std::string& title) {
    if (!tox_) {
        return false;
    }

    TOX_ERR_CONFERENCE_TITLE err = TOX_ERR_CONFERENCE_TITLE_OK;

    const bool ok = tox_conference_set_title(
        tox_, conference_number, reinterpret_cast<const uint8_t*>(title.data()),
        title.size(), &err);

    return ok && (err == TOX_ERR_CONFERENCE_TITLE_OK);
}

std::string ToxCoreWrapper::GetConferenceTitle(
    uint32_t conference_number) const {
    if (!tox_) {
        return {};
    }
    TOX_ERR_CONFERENCE_TITLE err = TOX_ERR_CONFERENCE_TITLE_OK;
    size_t title_size =
        tox_conference_get_title_size(tox_, conference_number, &err);
    if (err != TOX_ERR_CONFERENCE_TITLE_OK || title_size == 0) {
        return {};
    }
    std::string title(title_size, '\0');
    if (!tox_conference_get_title(tox_, conference_number,
                                  reinterpret_cast<uint8_t*>(title.data()),
                                  &err) ||
        err != TOX_ERR_CONFERENCE_TITLE_OK) {
        return {};
    }

    return title;
}

uint32_t ToxCoreWrapper::GetConferencePeerCount(
    uint32_t conference_number) const {
    if (!tox_) {
        return 0;
    }

    TOX_ERR_CONFERENCE_PEER_QUERY err = TOX_ERR_CONFERENCE_PEER_QUERY_OK;
    uint32_t count = tox_conference_peer_count(tox_, conference_number, &err);
    if (err != TOX_ERR_CONFERENCE_PEER_QUERY_OK) {
        return 0;
    }
    return count;
}

std::string ToxCoreWrapper::GetConferencePeerName(uint32_t conference_number,
                                                  uint32_t peer_number) const {
    if (!tox_) {
        return {};
    }

    TOX_ERR_CONFERENCE_PEER_QUERY err = TOX_ERR_CONFERENCE_PEER_QUERY_OK;

    size_t name_size = tox_conference_peer_get_name_size(
        tox_, conference_number, peer_number, &err);

    if (err != TOX_ERR_CONFERENCE_PEER_QUERY_OK || name_size == 0) {
        return {};
    }

    std::string name(name_size, '\0');
    if (!tox_conference_peer_get_name(tox_, conference_number, peer_number,
                                      reinterpret_cast<uint8_t*>(name.data()),
                                      &err) ||
        err != TOX_ERR_CONFERENCE_PEER_QUERY_OK) {
        return {};
    }

    return name;
}

std::string ToxCoreWrapper::GetConferencePeerPublicKeyHex(
    uint32_t conference_number, uint32_t peer_number) const {
    if (!tox_) {
        return {};
    }

    TOX_ERR_CONFERENCE_PEER_QUERY err = TOX_ERR_CONFERENCE_PEER_QUERY_OK;

    std::array<uint8_t, TOX_PUBLIC_KEY_SIZE> public_key{};

    if (!tox_conference_peer_get_public_key(
            tox_, conference_number, peer_number, public_key.data(), &err) ||
        err != TOX_ERR_CONFERENCE_PEER_QUERY_OK) {
        return {};
    }

    return EncodeHexUpper(public_key.data(), public_key.size());
}

bool ToxCoreWrapper::SendConferenceMessage(uint32_t conference_number,
                                           const std::string& message) {
    if (!tox_) {
        return false;
    }

    TOX_ERR_CONFERENCE_SEND_MESSAGE err = TOX_ERR_CONFERENCE_SEND_MESSAGE_OK;

    const bool ok = tox_conference_send_message(
        tox_, conference_number, TOX_MESSAGE_TYPE_NORMAL,
        reinterpret_cast<const uint8_t*>(message.data()), message.size(), &err);

    return ok && (err == TOX_ERR_CONFERENCE_SEND_MESSAGE_OK);
}

// =============== Conference Callback Management ===============

void ToxCoreWrapper::SetOnConferenceInvite(ConferenceInviteCb cb) {
    onConferenceInvite_ = std::move(cb);
    RefreshCallbacks_();
}

void ToxCoreWrapper::SetOnConferenceConnected(ConferenceConnectedCb cb) {
    onConferenceConnected_ = std::move(cb);
    RefreshCallbacks_();
}

void ToxCoreWrapper::SetOnConferenceMessage(ConferenceMessageCb cb) {
    onConferenceMessage_ = std::move(cb);
    RefreshCallbacks_();
}

void ToxCoreWrapper::SetOnConferenceTitle(ConferenceTitleCb cb) {
    onConferenceTitle_ = std::move(cb);
    RefreshCallbacks_();
}

void ToxCoreWrapper::SetOnConferencePeerName(ConferencePeerNameCb cb) {
    onConferencePeerName_ = std::move(cb);
    RefreshCallbacks_();
}

void ToxCoreWrapper::SetOnConferencePeerListChanged(
    ConferencePeerListChangedCb cb) {
    onConferencePeerListChanged_ = std::move(cb);
    RefreshCallbacks_();
}

// =============== video and audio call management ===============

int ToxCoreWrapper::Call(uint32_t friend_number, const CallOptions& options) {
    if (!av_) {
        return -1;
    }

    TOXAV_ERR_CALL err = TOXAV_ERR_CALL_OK;

    const bool ok = toxav_call(
        av_, friend_number,
        options.audio_enabled ? options.audio_bitrate_kbps : 0,
        options.video_enabled ? options.video_bitrate_kbps : 0, &err);

    return (ok && err == TOXAV_ERR_CALL_OK) ? 0 : -1;
}

int ToxCoreWrapper::Answer(uint32_t friend_number, const CallOptions& options) {
    if (!av_) {
        return -1;
    }

    TOXAV_ERR_ANSWER err = TOXAV_ERR_ANSWER_OK;

    const bool ok = toxav_answer(
        av_, friend_number,
        options.audio_enabled ? options.audio_bitrate_kbps : 0,
        options.video_enabled ? options.video_bitrate_kbps : 0, &err);

    return (ok && err == TOXAV_ERR_ANSWER_OK) ? 0 : -1;
}

int ToxCoreWrapper::Hangup(uint32_t friend_number) {
    if (!av_) {
        return -1;
    }

    TOXAV_ERR_CALL_CONTROL err = TOXAV_ERR_CALL_CONTROL_OK;
    const bool ok =
        toxav_call_control(av_, friend_number, TOXAV_CALL_CONTROL_CANCEL, &err);

    return (ok && err == TOXAV_ERR_CALL_CONTROL_OK) ? 0 : -1;
}

int ToxCoreWrapper::SendAudioFrame(uint32_t friend_number, const int16_t* pcm,
                                   size_t sample_count,
                                   const AudioFrameParams& params) {
    if (!av_) {
        return -1;
    }

    TOXAV_ERR_SEND_FRAME err = TOXAV_ERR_SEND_FRAME_OK;

    const bool ok =
        toxav_audio_send_frame(av_, friend_number, pcm, sample_count,
                               params.channels, params.sampling_rate, &err);

    return (ok && err == TOXAV_ERR_SEND_FRAME_OK) ? 0 : -1;
}

int ToxCoreWrapper::SendVideoFrame(uint32_t friend_number, uint16_t width,
                                   uint16_t height, const uint8_t* y,
                                   const uint8_t* u, const uint8_t* v) {
    if (!av_) {
        return -1;
    }
    TOXAV_ERR_SEND_FRAME err = TOXAV_ERR_SEND_FRAME_OK;
    const bool ok = toxav_video_send_frame(av_, friend_number, width, height, y,
                                           u, v, &err);
    return (ok && err == TOXAV_ERR_SEND_FRAME_OK) ? 0 : -1;
}

void ToxCoreWrapper::SetOnCall(CallCb cb) {
    onCall_ = std::move(cb);
    RefreshCallbacks_();
}

void ToxCoreWrapper::SetOnCallState(CallStateCb cb) {
    onCallState_ = std::move(cb);
    RefreshCallbacks_();
}

void ToxCoreWrapper::SetOnAudioFrame(AudioFrameCb cb) {
    onAudioFrame_ = std::move(cb);
    RefreshCallbacks_();
}

void ToxCoreWrapper::SetOnVideoFrame(VideoFrameCb cb) {
    onVideoFrame_ = std::move(cb);
    RefreshCallbacks_();
}

// ==================== Private Helper Methods ====================

void ToxCoreWrapper::EnsureTox_() const {
    if (!tox_) {
        throw std::logic_error("Tox instance is null");
    }
}

void ToxCoreWrapper::InitAv_() {
    TOXAV_ERR_NEW err = TOXAV_ERR_NEW_OK;
    av_ = toxav_new(tox_, &err);
    if (!av_ || err != TOXAV_ERR_NEW_OK) {
        throw std::runtime_error("toxav_new failed");
    }
    RefreshCallbacks_();
}

void ToxCoreWrapper::RefreshCallbacks_() {
    if (!tox_) {
        return;
    }

    tox_callback_friend_request(
        tox_, onFriendRequest_ ? &ToxCoreWrapper::OnFriendRequestCb_ : nullptr);

    tox_callback_friend_connection_status(
        tox_, onFriendConnectionStatus_
                  ? &ToxCoreWrapper::OnFriendConnectionStatusCb_
                  : nullptr);

    tox_callback_friend_message(
        tox_, onFriendMessage_ ? &ToxCoreWrapper::OnFriendMessageCb_ : nullptr);

    tox_callback_file_recv(
        tox_, onFileRecv_ ? &ToxCoreWrapper::OnFileRecvCb_ : nullptr);

    tox_callback_file_recv_control(
        tox_,
        onFileRecvControl_ ? &ToxCoreWrapper::OnFileRecvControlCb_ : nullptr);

    tox_callback_file_chunk_request(
        tox_,
        onFileChunkRequest_ ? &ToxCoreWrapper::OnFileChunkRequestCb_ : nullptr);

    tox_callback_file_recv_chunk(
        tox_, onFileRecvChunk_ ? &ToxCoreWrapper::OnFileRecvChunkCb_ : nullptr);

    tox_callback_conference_invite(
        tox_,
        onConferenceInvite_ ? &ToxCoreWrapper::OnConferenceInviteCb_ : nullptr);

    tox_callback_conference_connected(
        tox_, onConferenceConnected_ ? &ToxCoreWrapper::OnConferenceConnectedCb_
                                     : nullptr);
    tox_callback_conference_message(
        tox_, onConferenceMessage_ ? &ToxCoreWrapper::OnConferenceMessageCb_
                                   : nullptr);
    tox_callback_conference_title(
        tox_,
        onConferenceTitle_ ? &ToxCoreWrapper::OnConferenceTitleCb_ : nullptr);

    tox_callback_conference_peer_name(
        tox_, onConferencePeerName_ ? &ToxCoreWrapper::OnConferencePeerNameCb_
                                    : nullptr);
    tox_callback_conference_peer_list_changed(
        tox_, onConferencePeerListChanged_
                  ? &ToxCoreWrapper::OnConferencePeerListChangedCb_
                  : nullptr);

    if (av_) {
        toxav_callback_call(av_, onCall_ ? &ToxCoreWrapper::OnCallCb_ : nullptr,
                            this);
        toxav_callback_call_state(
            av_, onCallState_ ? &ToxCoreWrapper::OnCallStateCb_ : nullptr,
            this);
        toxav_callback_audio_receive_frame(
            av_, onAudioFrame_ ? &ToxCoreWrapper::OnAudioFrameCb_ : nullptr,
            this);
        toxav_callback_video_receive_frame(
            av_, onVideoFrame_ ? &ToxCoreWrapper::OnVideoFrameCb_ : nullptr,
            this);
    }
}

// ================ Static callback bridge functions ================

void ToxCoreWrapper::OnFriendRequestCb_(Tox* tox,
                                        const Tox_Public_Key public_key,
                                        const uint8_t message[], size_t length,
                                        void* opaque) {
    auto* self = static_cast<ToxCoreWrapper*>(opaque);
    if (!self || !self->onFriendRequest_) {
        return;
    }

    if (!public_key) {
        return;
    }

    const std::string public_key_hex =
        EncodeHexUpper(public_key, TOX_PUBLIC_KEY_SIZE);
    const std::string msg(reinterpret_cast<const char*>(message), length);

    self->onFriendRequest_(public_key_hex, msg);
}

void ToxCoreWrapper::OnFriendConnectionStatusCb_(
    Tox* tox, Tox_Friend_Number friend_number, Tox_Connection connection_status,
    void* opaque) {
    auto* self = static_cast<ToxCoreWrapper*>(opaque);
    if (!self || !self->onFriendConnectionStatus_) {
        return;
    }
    self->onFriendConnectionStatus_(friend_number, connection_status);
}

void ToxCoreWrapper::OnFriendMessageCb_(Tox* tox,
                                        Tox_Friend_Number friend_number,
                                        Tox_Message_Type type,
                                        const uint8_t message[], size_t length,
                                        void* opaque) {
    auto* self = static_cast<ToxCoreWrapper*>(opaque);
    if (!self || !self->onFriendMessage_) {
        return;
    }

    const std::string msg(reinterpret_cast<const char*>(message), length);
    self->onFriendMessage_(friend_number, type, msg);
}

void ToxCoreWrapper::OnFileRecvCb_(Tox* tox, Tox_Friend_Number friend_number,
                                   Tox_File_Number file_number, uint32_t kind,
                                   uint64_t file_size, const uint8_t filename[],
                                   size_t filename_length, void* opaque) {
    auto* self = static_cast<ToxCoreWrapper*>(opaque);
    if (!self || !self->onFileRecv_) {
        return;
    }

    if (kind != TOX_FILE_KIND_DATA) {
        return;
    }

    std::string file_name(reinterpret_cast<const char*>(filename),
                          filename_length);

    self->onFileRecv_(friend_number, file_number, file_name, file_size);
}

void ToxCoreWrapper::OnFileRecvControlCb_(Tox* tox,
                                          Tox_Friend_Number friend_number,
                                          Tox_File_Number file_number,
                                          Tox_File_Control control,
                                          void* opaque) {
    auto* self = static_cast<ToxCoreWrapper*>(opaque);
    if (!self || !self->onFileRecvControl_) {
        return;
    }

    self->onFileRecvControl_(friend_number, file_number, control);
}

void ToxCoreWrapper::OnFileChunkRequestCb_(Tox* tox,
                                           Tox_Friend_Number friend_number,
                                           Tox_File_Number file_number,
                                           uint64_t position, size_t length,
                                           void* opaque) {
    auto* self = static_cast<ToxCoreWrapper*>(opaque);
    if (!self || !self->onFileChunkRequest_) {
        return;
    }

    self->onFileChunkRequest_(friend_number, file_number, position, length);
}

void ToxCoreWrapper::OnFileRecvChunkCb_(Tox* tox,
                                        Tox_Friend_Number friend_number,
                                        Tox_File_Number file_number,
                                        uint64_t position, const uint8_t data[],
                                        size_t length, void* opaque) {
    auto* self = static_cast<ToxCoreWrapper*>(opaque);
    if (!self || !self->onFileRecvChunk_) {
        return;
    }

    self->onFileRecvChunk_(friend_number, file_number, position, data, length);
}

void ToxCoreWrapper::OnConferenceInviteCb_(Tox* tox,
                                           Tox_Friend_Number friend_number,
                                           Tox_Conference_Type type,
                                           const uint8_t cookie[],
                                           size_t length, void* opaque) {
    auto* self = static_cast<ToxCoreWrapper*>(opaque);
    if (!self || !self->onConferenceInvite_) {
        return;
    }
    std::vector<uint8_t> cookieData(cookie, cookie + length);
    self->onConferenceInvite_(friend_number, type, cookieData);
}

void ToxCoreWrapper::OnConferenceConnectedCb_(
    Tox* tox, Tox_Conference_Number conference_number, void* opaque) {
    auto* self = static_cast<ToxCoreWrapper*>(opaque);
    if (!self || !self->onConferenceConnected_) {
        return;
    }
    self->onConferenceConnected_(conference_number);
}

void ToxCoreWrapper::OnConferenceMessageCb_(
    Tox* tox, Tox_Conference_Number conference_number,
    Tox_Conference_Peer_Number peer_number, Tox_Message_Type type,
    const uint8_t message[], size_t length, void* opaque) {
    auto* self = static_cast<ToxCoreWrapper*>(opaque);
    if (!self || !self->onConferenceMessage_) {
        return;
    }
    const std::string msg(reinterpret_cast<const char*>(message), length);
    self->onConferenceMessage_(conference_number, peer_number, type, msg);
}

void ToxCoreWrapper::OnConferenceTitleCb_(
    Tox* tox, Tox_Conference_Number conference_number,
    Tox_Conference_Peer_Number peer_number, const uint8_t title[],
    size_t length, void* opaque) {
    auto* self = static_cast<ToxCoreWrapper*>(opaque);
    if (!self || !self->onConferenceTitle_) {
        return;
    }
    const std::string t(reinterpret_cast<const char*>(title), length);
    self->onConferenceTitle_(conference_number, peer_number, t);
}

void ToxCoreWrapper::OnConferencePeerNameCb_(
    Tox* tox, Tox_Conference_Number conference_number,
    Tox_Conference_Peer_Number peer_number, const uint8_t name[], size_t length,
    void* opaque) {
    auto* self = static_cast<ToxCoreWrapper*>(opaque);
    if (!self || !self->onConferencePeerName_) {
        return;
    }
    const std::string n(reinterpret_cast<const char*>(name), length);
    self->onConferencePeerName_(conference_number, peer_number, n);
}

void ToxCoreWrapper::OnConferencePeerListChangedCb_(
    Tox* tox, Tox_Conference_Number conference_number, void* opaque) {
    auto* self = static_cast<ToxCoreWrapper*>(opaque);
    if (!self || !self->onConferencePeerListChanged_) {
        return;
    }
    self->onConferencePeerListChanged_(conference_number);
}

void ToxCoreWrapper::OnCallCb_(ToxAV* av, Tox_Friend_Number friend_number,
                               bool audio_enabled, bool video_enabled,
                               void* opaque) {
    auto* self = static_cast<ToxCoreWrapper*>(opaque);
    if (!self || !self->onCall_) {
        return;
    }
    self->onCall_(friend_number, audio_enabled, video_enabled);
}

void ToxCoreWrapper::OnCallStateCb_(ToxAV* av, Tox_Friend_Number friend_number,
                                    uint32_t state, void* opaque) {
    auto* self = static_cast<ToxCoreWrapper*>(opaque);
    if (!self || !self->onCallState_) {
        return;
    }
    self->onCallState_(friend_number,
                       static_cast<TOXAV_FRIEND_CALL_STATE>(state));
}

void ToxCoreWrapper::OnAudioFrameCb_(ToxAV* av, Tox_Friend_Number friend_number,
                                     const int16_t pcm[], size_t sample_count,
                                     uint8_t channels, uint32_t sampling_rate,
                                     void* opaque) {
    auto* self = static_cast<ToxCoreWrapper*>(opaque);
    if (!self || !self->onAudioFrame_) {
        return;
    }
    self->onAudioFrame_(friend_number, pcm, sample_count, channels,
                        sampling_rate);
}

void ToxCoreWrapper::OnVideoFrameCb_(ToxAV* av, Tox_Friend_Number friend_number,
                                     uint16_t width, uint16_t height,
                                     const uint8_t y[], const uint8_t u[],
                                     const uint8_t v[], int32_t ystride,
                                     int32_t ustride, int32_t vstride,
                                     void* opaque) {
    auto* self = static_cast<ToxCoreWrapper*>(opaque);
    if (!self || !self->onVideoFrame_) {
        return;
    }

    // Handling stride: If stride != width, data needs to be copied.
    // To simplify, a temporary buffer is created here.
    std::vector<uint8_t> yPlane(width * height);
    std::vector<uint8_t> uPlane((width / 2) * (height / 2));
    std::vector<uint8_t> vPlane((width / 2) * (height / 2));

    // Copy Y plane
    for (uint16_t i = 0; i < height; ++i) {
        memcpy(yPlane.data() + i * width, y + i * ystride, width);
    }

    // Copy U plane
    for (uint16_t i = 0; i < height / 2; ++i) {
        memcpy(uPlane.data() + i * (width / 2), u + i * ustride, width / 2);
    }

    // Copy V plane
    for (uint16_t i = 0; i < height / 2; ++i) {
        memcpy(vPlane.data() + i * (width / 2), v + i * vstride, width / 2);
    }

    self->onVideoFrame_(friend_number, width, height, yPlane.data(),
                        uPlane.data(), vPlane.data());
}

}  // namespace ToxCore
