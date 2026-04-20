#include "core/toxcore_wrapper.h"

#include <array>
#include <stdexcept>

namespace ToxCore {
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
    : tox_(other.tox_), av_(other.av_) {
    other.tox_ = nullptr;
    other.av_ = nullptr;
}

ToxCoreWrapper& ToxCoreWrapper::operator=(ToxCoreWrapper&& other) noexcept {
    if (this == &other) {
        return *this;  // Self-assignment check
    }

    if (tox_) {
        tox_kill(tox_);
        tox_ = nullptr;
    }

    if (av_) {
        toxav_kill(av_);
        av_ = nullptr;
    }

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

void ToxCoreWrapper::InitAv_() {
    TOXAV_ERR_NEW err = TOXAV_ERR_NEW_OK;
    av_ = toxav_new(tox_, &err);
    if (!av_ || err != TOXAV_ERR_NEW_OK) {
        throw std::runtime_error("toxav_new failed");
    }
    RefreshCallbacks_();
}

void ToxCoreWrapper::RefreshCallbacks_() {}

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

    const uint32_t coreMs = tox_iteration_interval(tox_);
    const uint32_t avMs = av_ ? toxav_iteration_interval(av_) : coreMs;

    return std::min(coreMs, avMs);
}

std::vector<uint8_t> ToxCoreWrapper::GetSavedata() const {
    if (!tox_) {
        throw std::logic_error("Tox instance is null");
    }

    const size_t data_size = tox_get_savedata_size(tox_);
    if (data_size == 0) {
        return {};
    }

    std::vector<uint8_t> savedata(data_size);
    tox_get_savedata(tox_, savedata.data());
    return savedata;
}

std::string ToxCoreWrapper::GetSelfAddressHex() const {
    if (!tox_) {
        throw std::logic_error("Tox instance is null");
    }

    std::array<uint8_t, TOX_ADDRESS_SIZE> address{};
    tox_self_get_address(tox_, address.data());

    return EncodeHexUpper(address.data(), address.size());
}

uint32_t ToxCoreWrapper::AddFriend(const std::string& tox_id_hex,
                                   const std::string& message) {
    if (!tox_) {
        throw std::logic_error("Tox instance is null");
    }

    if (message.empty()) {
        throw std::invalid_argument("Friend request message cannot be empty");
    }

    if (message.size() > TOX_MAX_FRIEND_REQUEST_LENGTH) {
        throw std::invalid_argument(
            "Friend request message exceeds maximum length");
    }

    const auto address = DecodeHexFixed<TOX_ADDRESS_SIZE>(tox_id_hex);

    TOX_ERR_FRIEND_ADD err = TOX_ERR_FRIEND_ADD_OK;
    const uint32_t friendNum = tox_friend_add(
        tox_, address.data(), reinterpret_cast<const uint8_t*>(message.data()),
        message.size(), &err);

    if (err != TOX_ERR_FRIEND_ADD_OK) {
        throw std::runtime_error("tox_friend_add failed, err=" +
                                 std::to_string(static_cast<int>(err)));
    }

    return friendNum;
}
}  // namespace ToxCore