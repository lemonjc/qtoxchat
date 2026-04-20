#pragma once

#include <toxav/toxav.h>
#include <toxcore/tox.h>

#include <cstdint>
#include <string>
#include <vector>

namespace ToxCore {
std::string EncodeHexUpper(const uint8_t* data, size_t length);

class ToxCoreWrapper {
public:
    // ==================== Lifecycle Management ====================
    /**
     * @brief Ctor: create a new ToxCore instance (gernate a identity)
     */
    explicit ToxCoreWrapper();

    /**
     * @brief Ctor: restore a ToxCore instance from savedata
     * @param savedata The savedata to restore from
     */
    explicit ToxCoreWrapper(const std::vector<uint8_t>& savedata);

    ToxCoreWrapper(const ToxCoreWrapper&) = delete;
    ToxCoreWrapper& operator=(const ToxCoreWrapper&) = delete;

    ToxCoreWrapper(ToxCoreWrapper&&) noexcept;
    ToxCoreWrapper& operator=(ToxCoreWrapper&&) noexcept;

    /**
     * @brief Dtor: release the ToxCore instance
     */
    ~ToxCoreWrapper();

    // ==================== Core Loop Management ====================

    /**
     * @brief Main loop iteration function: Handles network events (must be
     * called periodically)\n
     *
     * This function handles:
     *
     * - Sending and receiving network packets
     *
     * - Maintaining DHT nodes
     *
     * - Updating friend connection status
     *
     * - Triggering various event callbacks
     */
    void Iterate();

    /**
     * @brief Get the recommended iteration interval (milliseconds)
     * @return Recommended Iterate() call interval (typically 50-1000 ms)
     */
    uint32_t IterationIntervalMs() const;

    // ==================== Self Information Management ====================
    /**
     * @brief Obtain your own Tox ID (76-character hexadecimal string)
     * @return Tox ID (Format: 32-byte public key + 4-byte Nospam + 2-byte
     * checksum = 76 hex digits)
     */
    std::string GetSelfAddressHex() const;

    /**
     * @brief Retrieve Tox savedata (for persistent storage)
     * @return savedata binary data (includes all states such as identity,
     * friend list, groups, etc.)
     */
    std::vector<uint8_t> GetSavedata() const;

    // ==================== Friend Management ====================

    /**
     * @brief Send friend request (add friend)
     * @param tox_id_hex The other party's Tox ID (76-digit hexadecimal string)
     * @param message Additional message (friend request description)
     * @return Friend number, returns UINT32_MAX on failure
     */
    uint32_t AddFriend(const std::string& tox_id_hex,
                       const std::string& message);

    /**
     * @brief Add Friend Without Request (for scenarios where the other party's
     * public key is known, such as group chat members)
     * @param public_key_hex The other party's public key (64-bit hexadecimal
     * string)
     * @return Friend number, returns UINT32_MAX on failure
     */
    uint32_t AddFriendNoRequest(const std::string& public_key_hex);

private:
    /**
     * @brief Initialize ToxAV (audio and video calling module)\n
     *
     * This method is called after the Tox instance is created to initialize
     * audio and video functionality
     */
    void InitAv_();

    /**
     * @brief Refresh all callback function registrations (binding C++ callbacks
     * to toxcore C callbacks)\n
     *
     * This method is called during construction and move assignment to ensure
     * proper binding of the callback.
     */
    void RefreshCallbacks_();

private:
    Tox* tox_{nullptr};
    ToxAV* av_{nullptr};
};
}  // namespace ToxCore
