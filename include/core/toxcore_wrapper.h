#pragma once

#include <toxav/toxav.h>
#include <toxcore/tox.h>

#include <cstdint>
#include <functional>
#include <string>
#include <vector>

namespace ToxCore {
// ==================== Callback Type Definitions ====================

// Friend-related callback type definitions
using FriendRequestCb = std::function<void(const std::string& public_key_hex,
                                           const std::string& message)>;
using FriendConnectionStatusCb =
    std::function<void(uint32_t friend_number, TOX_CONNECTION status)>;
using FriendMessageCb = std::function<void(
    uint32_t friend_number, TOX_MESSAGE_TYPE type, const std::string& message)>;

// File transfer-related callback type definitions
using FileReciveCb =
    std::function<void(uint32_t friend_number, uint32_t file_number,
                       const std::string& filename, uint64_t filesize)>;
using FileRecvControlCb = std::function<void(
    uint32_t friend_number, uint32_t file_number, TOX_FILE_CONTROL control)>;
using FileChunkRequestCb =
    std::function<void(uint32_t friend_number, uint32_t file_number,
                       uint64_t position, uint64_t length)>;
using FileChunkRecvCb =
    std::function<void(uint32_t friend_number, uint32_t file_number,
                       uint64_t position, const std::vector<uint8_t>& data)>;

// Conference-related callback type definitions
using ConferenceInviteCb =
    std::function<void(uint32_t friend_number, TOX_CONFERENCE_TYPE type,
                       const std::vector<uint8_t>& cookie)>;

/**
 * @brief Encodes a byte buffer as an uppercase hexadecimal string.
 * @param data Pointer to the input byte buffer; the first 'length' bytes are
 * encoded.
 * @param length Number of bytes to encode from data.
 * @return A std::string containing uppercase hexadecimal characters (two
 * characters per byte). Returns an empty string if length is zero.
 */
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

    /**
     * @brief Delete a friend
     * @param friend_number Friend number (index in friend list)
     */
    void DeleteFriend(uint32_t friend_number);

    /**
     * @brief Get friend list
     * @return List of all friends' IDs
     */
    std::vector<uint32_t> GetFriendList() const;

    /**
     * @brief Check if friend exists
     * @param friend_number Friend number (index in friend list)
     * @return True if the friend exists, false otherwise
     */
    bool FriendExists(uint32_t friend_number) const;

    /**
     * @brief Get the connection status of a friend
     * @param friend_number Friend number (index in friend list)
     * @return Connection status of the friend
     */
    TOX_CONNECTION GetFriendConnectionStatus(uint32_t friend_number) const;

    /**
     * @brief Set callback for friend connection status changes
     * @param cb Callback function to handle friend connection status changes
     */
    void SetOnFriendConnectionStatus(FriendConnectionStatusCb cb);

    // ==================== Message Management ====================

    /**
     * @brief Send a message to a friend
     * @param friend_number Friend number
     * @param message Message content to send
     * @param type Message type (normal, action, etc.)
     * @return Message ID (for tracking), returns 0 on failure
     */
    uint32_t SendFriendMessage(uint32_t friend_number,
                               const std::string& message,
                               TOX_MESSAGE_TYPE type = TOX_MESSAGE_TYPE_NORMAL);

    void SetOnFriendMessage(FriendMessageCb cb);

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
    // ==================== Callback Function Storage ====================
    // friend-related callbacks

    FriendConnectionStatusCb onFriendConnectionStatus_{nullptr};

private:
    Tox* tox_{nullptr};
    ToxAV* av_{nullptr};
};
}  // namespace ToxCore
