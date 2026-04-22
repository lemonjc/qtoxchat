#pragma once

#include <toxav/toxav.h>
#include <toxcore/tox.h>

#include <cstdint>
#include <filesystem>
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
using ConferenceConnectedCb = std::function<void(uint32_t conference_number)>;
using ConferenceMessageCb =
    std::function<void(uint32_t conference_number, uint32_t peer_number,
                       TOX_MESSAGE_TYPE type, const std::string& message)>;
using ConferenceTitleCb =
    std::function<void(uint32_t conference_number, uint32_t peer_number,
                       const std::string& title)>;
using ConferencePeerNameCb = std::function<void(
    uint32_t conference_number, uint32_t peer_number, const std::string& name)>;
using ConferencePeerListChangedCb =
    std::function<void(uint32_t conference_number)>;

// video and audio call-related callback type definitions
using CallCb = std::function<void(uint32_t friend_number, bool audio_enabled,
                                  bool video_enabled)>;
using CallStateCb = std::function<void(uint32_t friend_number, uint32_t state)>;
using AudioFrameCb = std::function<void(
    uint32_t friend_number, const int16_t* pcm, size_t sample_count,
    uint8_t channels, uint32_t sampling_rate)>;
using VideoFrameCb =
    std::function<void(uint32_t friend_number, uint16_t width, uint16_t height,
                       const uint8_t* y, const uint8_t* u, const uint8_t* v,
                       int32_t ystride, int32_t u_stride, int32_t vstride)>;

// ==================== Options Struct Definition ====================

struct CallOptions {
    bool audio_enabled{true};         // Whether to enable audio in calls
    bool video_enabled{false};        // Whether to enable video in calls
    uint32_t audio_bitrate_kbps{48};  // Audio bitrate in kbps (e.g., 48 kbps)
    uint32_t video_bitrate_kbps{
        5000};  // Video bitrate in kbps (e.g., 5000 kbps)
};

struct AudioFrameParams {
    uint8_t channels{1};  // Number of audio channels (e.g., 1=mono, 2=stereo)
    uint32_t sampling_rate{48000};  // Sampling rate in Hz (e.g., 48000 Hz)
};

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

    /**
     * @brief Set the Nospam value for the user
     * @param nospam The Nospam value to set
     */
    void SetSelfNospam(uint32_t nospam);

    /**
     * @brief Get the current Nospam value for the user
     * @return The current Nospam value
     */
    uint32_t GetSelfNospam() const;

    /**
     * @brief Set the user's display name
     * @param name The display name to set
     */
    void SetSelfName(const std::string& name);

    /**
     * @brief Get the user's name
     * @return The current name
     */
    std::string GetSelfName() const;

    /**
     * @brief Set the user's status message
     * @param status_message The status message to set
     */
    void SetSelfStatusMessage(const std::string& status_message);

    /**
     * @brief Get the user's status message
     * @return The current status message
     */
    std::string GetSelfStatusMessage() const;

    /**
     * @brief Set the user's online status
     * @param status The online status to set (e.g., NONE, AWAY, BUSY)
     */
    void SetSelfStatus(Tox_User_Status status);

    /**
     * @brief Get the user's current online status
     * @return The current online status
     */
    Tox_User_Status GetSelfStatus() const;

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
     * @brief Get friend's public key in hexadecimal format
     * @param friend_number Friend number (index in friend list)
     * @return Friend's public key as a hexadecimal string
     */
    std::string GetFriendPublicKeyHex(uint32_t friend_number) const;

    /**
     * @brief Get friend's display name
     * @param friend_number Friend number (index in friend list)
     * @return Friend's display name
     */
    std::string GetFriendName(uint32_t friend_number) const;

    /**
     * @brief Get the connection status of a friend
     * @param friend_number Friend number (index in friend list)
     * @return Connection status of the friend
     */
    TOX_CONNECTION GetFriendConnectionStatus(uint32_t friend_number) const;

    /**
     * @brief Set callback for incoming friend requests
     * @param cb Callback function to handle incoming friend requests
     */
    void SetOnFriendRequest(FriendRequestCb cb);

    /**
     * @brief Set callback for friend connection status changes
     * @param cb Callback function to handle friend connection status changes
     */
    void SetOnFriendConnectionStatus(FriendConnectionStatusCb cb);

    // ==================== Message Management ====================

    /**
     * @brief Send a message to a friend
     * @param friend_number Friend number
     * @param message Message content to send (UTF-8 string)
     * @param type Message type (normal, action, etc.)
     * @return Message ID (for tracking), returns 0 on failure
     */
    uint32_t SendFriendMessage(uint32_t friend_number,
                               const std::string& message,
                               TOX_MESSAGE_TYPE type = TOX_MESSAGE_TYPE_NORMAL);

    /**
     * @brief Set callback for incoming messages from friends
     * @param cb Callback function to handle incoming friend messages
     */
    void SetOnFriendMessage(FriendMessageCb cb);

    // ==================== Network Management ====================

    /**
     * @brief Connect to a bootstrap node (for NAT traversal)
     * @param address IP address of the bootstrap node
     * @param port Port number of the bootstrap node
     * @param public_key_hex Public key of the bootstrap node (64-digit
     * hexadecimal string)
     */
    void Bootstrap(const std::string& address, uint16_t port,
                   const std::string& public_key_hex);

    /**
     * @brief Add a TCP relay node (for improved network connectivity)
     * @param address IP address of the TCP relay node
     * @param port Port number of the TCP relay node
     * @param public_key_hex Public key of the TCP relay node (64-digit
     * hexadecimal string)
     */
    void AddTcpRelay(const std::string& address, uint16_t port,
                     const std::string& public_key_hex);

    /**
     * @brief Get the current connection status (whether joined the DHT network)
     * @return Connection status (NONE, TCP, UDP, etc.)
     */
    TOX_CONNECTION GetSelfConnectionStatus() const;

    // ==================== File Transfer Management ====================

    /**
     * @brief Send a file to a friend
     * @param friend_number Friend number
     * @param file_path Path to the file to send
     * @param file_name_utf8 File name in UTF-8 format
     * @return File transfer ID (for tracking), returns UINT32_MAX on failure
     */
    uint32_t SendFile(uint32_t friend_number,
                      const std::filesystem::path& file_path,
                      const std::string& file_name_utf8);

    /**
     * @brief Control an ongoing file transfer (accept, reject, cancel)
     * @param friend_number Friend number
     * @param file_number File transfer ID
     * @param control Control action (accept, reject, cancel)
     * @return True if the control action was successfully sent, false otherwise
     */
    bool ControlFileTransfer(uint32_t friend_number, uint32_t file_number,
                             TOX_FILE_CONTROL control);

    /**
     * @brief Send a chunk of file data in response to a chunk request
     * @param friend_number Friend number
     * @param file_number File transfer ID
     * @param position Position in the file to send (offset)
     * @param data Chunk data to send
     * @return True if the file chunk was successfully sent, false otherwise
     */
    bool SendFileChunk(uint32_t friend_number, uint32_t file_number,
                       uint64_t position, const std::vector<uint8_t>& data);

    /**
     * @brief Set callback for incoming file transfers
     * @param cb Callback function to handle incoming file transfers
     */
    void SetOnFileRecive(FileReciveCb cb);

    /**
     * @brief Set callback for file transfer control events (accept, reject,
     * cancel)
     * @param cb Callback function to handle file transfer control events
     */
    void SetOnFileRecvControl(FileRecvControlCb cb);

    /**
     * @brief Set callback for file chunk requests
     * @param cb Callback function to handle file chunk requests
     */
    void SetOnFileChunkRequest(FileChunkRequestCb cb);

    /**
     * @brief Set callback for receiving file chunks
     * @param cb Callback function to handle received file chunks
     */
    void SetOnFileChunkRecv(FileChunkRecvCb cb);

    // ==================== Conference Management ====================

    /**
     * @brief Create a new conference (group chat)
     * @return Conference number (ID), returns UINT32_MAX on failure
     */
    uint32_t CreateConference();

    /**
     * @brief Delete a conference (leave the group chat)
     * @param conference_number Conference number (ID)
     * @return True if the conference was successfully deleted, false otherwise
     */
    bool DeleteConference(uint32_t conference_number);

    /**
     * @brief Invite a friend to join a conference
     * @param conference_number Conference number (ID)
     * @param friend_number Friend number to invite
     * @return True if the invitation was successfully sent, false otherwise
     */
    bool InviteFriendToConference(uint32_t conference_number,
                                  uint32_t friend_number);

    /**
     * @brief Join a conference
     * @param conference_number Conference number (ID)
     * @param cookie Cookie data for joining the conference
     * @return True if successfully joined, false otherwise
     */
    bool JoinConference(uint32_t conference_number,
                        const std::vector<uint8_t>& cookie);

    // ================= Conference Information Retrieval ===============

    /**
     * @brief Get the conference ID
     * @param conference_number Conference number (ID)
     * @return Conference ID as a byte vector
     */
    std::vector<uint8_t> GetConferenceId(uint32_t conference_number) const;

    /**
     * @brief Get the conference ID in hexadecimal format
     * @param conference_number Conference number (ID)
     * @return Conference ID as a hexadecimal string (if the conference exists),
     * std::nullopt if the conference does not exist
     */
    std::optional<uint32_t> GetConferenceByIdHex(
        const std::string& id_hex) const;

    /**
     * @brief Get the list of all conference numbers (IDs)
     * @return List of all conference numbers (IDs)
     */
    std::vector<uint32_t> GetConferenceList() const;

    // ============== Conference Information Management ===============

    /**
     * @brief Set the conference title
     * @param conference_number Conference number (ID)
     * @param title The title to set for the conference
     * @return True if the title was successfully set, false otherwise
     */
    bool SetConferenceTitle(uint32_t conference_number,
                            const std::string& title);

    /**
     * @brief Get the conference title
     * @param conference_number Conference number (ID)
     * @return Conference title as a string
     */
    std::string GetConferenceTitle(uint32_t conference_number) const;

    /**
     * @brief Get the number of peers in the conference
     * @param conference_number Conference number (ID)
     * @return Number of peers in the conference
     */
    uint32_t GetConferencePeerCount(uint32_t conference_number) const;

    /**
     * @brief Get the name of a peer in the conference
     * @param conference_number Conference number (ID)
     * @param peer_number Peer number (ID)
     * @return Peer name as a string
     */
    std::string GetConferencePeerName(uint32_t conference_number,
                                      uint32_t peer_number) const;

    /**
     * @brief Get the public key of a peer in the conference in hexadecimal
     * format
     * @param conference_number Conference number (ID)
     * @param peer_number Peer number (ID)
     * @return Peer public key as a hexadecimal string
     */
    std::string GetConferencePeerPublicKeyHex(uint32_t conference_number,
                                              uint32_t peer_number) const;

    /**
     * @brief Send a message to a conference
     * @param conference_number Conference number (ID)
     * @param message Message to send
     */
    void SendConferenceMessage(uint32_t conference_number,
                               const std::string& message);

    // =============== Conference Callback Management ===============

    /**
     * @brief Set callback for incoming conference invitations
     * @param cb Callback function to handle incoming conference invitations
     */
    void SetOnConferenceInvite(ConferenceInviteCb cb);

    /**
     * @brief Set callback for conference connection events
     * @param cb Callback function to handle conference connection events
     */
    void SetOnConferenceConnected(ConferenceConnectedCb cb);

    /**
     * @brief Set callback for incoming conference messages
     * @param cb Callback function to handle incoming conference messages
     */
    void SetOnConferenceMessage(ConferenceMessageCb cb);

    /**
     * @brief Set callback for conference title changes
     * @param cb Callback function to handle conference title changes
     */
    void SetOnConferenceTitle(ConferenceTitleCb cb);

    /**
     * @brief Set callback for conference peer name changes
     * @param cb Callback function to handle conference peer name changes
     */
    void SetOnConferencePeerName(ConferencePeerNameCb cb);

    /**
     * @brief Set callback for conference peer list changes (peers joining or
     * leaving)
     * @param cb Callback function to handle conference peer list changes
     */
    void SetOnConferencePeerListChanged(ConferencePeerListChangedCb cb);

    // =============== video and audio call management ===============

    /**
     * @brief Initiate a call to a friend
     * @param friend_number Friend number to call
     * @param options Call options (audio/video enabled, bitrate settings)
     * @return 0 if the call was successfully hung up, negative value on failure
     */
    int Call(uint32_t friend_number, const CallOptions& options = {});

    /**
     * @brief Answer an incoming call from a friend
     * @param friend_number Friend number to answer
     * @param options Call options (audio/video enabled, bitrate settings)
     * @return 0 if the call was successfully hung up, negative value on failure
     */
    int Answer(uint32_t friend_number, const CallOptions& options = {});

    /**
     * @brief Hang up an ongoing call with a friend
     * @param friend_number Friend number to hang up
     * @return 0 if the call was successfully hung up, negative value on failure
     */
    int Hangup(uint32_t friend_number);

    /**
     * @brief Send an audio frame to a friend during a call
     * @param friend_number Friend number to send the audio frame to
     * @param pcm PCM audio data (interleaved if multiple channels)
     * @param sample_count Number of audio samples in the PCM data
     * @param params Additional parameters for the audio frame (channels,
     * sampling rate)
     * @return 0 if the audio frame was successfully sent, negative value on
     * failure
     */
    int SendAudioFrame(uint32_t friend_number, const int16_t* pcm,
                       size_t sample_count,
                       const AudioFrameParams& params = {});

    /**
     * @brief Send a video frame to a friend during a call
     * @param friend_number Friend number to send the video frame to
     * @param width Width of the video frame in pixels
     * @param height Height of the video frame in pixels
     * @param y Y plane data (YUV420 format)
     * @param u U plane data (YUV420 format)
     * @param v V plane data (YUV420 format)
     * @return 0 if the video frame was successfully sent, negative value on
     * failure
     */
    int SendVideoFrame(uint32_t friend_number, uint16_t width, uint16_t height,
                       const uint8_t* y, const uint8_t* u, const uint8_t* v);

    /**
     * @brief Set callback for incoming calls from friends
     * @param cb Callback function to handle incoming calls
     */
    void SetOnCall(CallCb cb);

    /**
     * @brief Set callback for call state changes (e.g., ringing, connected,
     * ended)
     * @param cb Callback function to handle call state changes
     */
    void SetOnCallState(CallStateCb cb);

    /**
     * @brief Set callback for receiving audio frames from friends during calls
     * @param cb Callback function to handle incoming audio frames
     */
    void SetOnAudioFrame(AudioFrameCb cb);

    /**
     * @brief Set callback for receiving video frames from friends during calls
     * @param cb Callback function to handle incoming video frames
     */
    void SetOnVideoFrame(VideoFrameCb cb);

private:
    // ==================== Private Helper Methods ====================
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

    // ================ Static callback bridge functions ================

    // Friend-related static callback functions
    static void OnFriendRequestCb_(Tox* tox, const Tox_Public_Key public_key,
                                   const uint8_t message[], size_t length,
                                   void* opaque);
    static void OnFriendConnectionStatusCb_(Tox* tox,
                                            Tox_Friend_Number friend_number,
                                            Tox_Connection connection_status,
                                            void* opaque);
    static void OnFriendMessageCb_(Tox* tox, Tox_Friend_Number friend_number,
                                   Tox_Message_Type type,
                                   const uint8_t message[], size_t length,
                                   void* opaque);

    // File transfer-related static callback functions
    static void OnFileReciveCb_(Tox* tox, Tox_Friend_Number friend_number,
                                Tox_File_Number file_number, uint32_t kind,
                                uint64_t file_size, const uint8_t filename[],
                                size_t filename_length, void* opaque);

    static void OnFileRecvControlCb_(Tox* tox, Tox_Friend_Number friend_number,
                                     Tox_File_Number file_number,
                                     Tox_File_Control control, void* opaque);

    static void OnFileChunkRequestCb_(Tox* tox, Tox_Friend_Number friend_number,
                                      Tox_File_Number file_number,
                                      uint64_t position, size_t length,
                                      void* opaque);

    static void OnFileRecvChunkCb(Tox* tox, Tox_Friend_Number friend_number,
                                  Tox_File_Number file_number,
                                  uint64_t position, const uint8_t data[],
                                  size_t length, void* opaque);

    // Conference-related static callback functions
    static void OnConferenceInviteCb_(Tox* tox, Tox_Friend_Number friend_number,
                                      Tox_Conference_Type type,
                                      const uint8_t cookie[], size_t length,
                                      void* opaque);

    static void OnConferenceConnectedCb_(
        Tox* tox, Tox_Conference_Number conference_number, void* opaque);

    static void OnConferenceMessageCb_(Tox* tox,
                                       Tox_Conference_Number conference_number,
                                       Tox_Conference_Peer_Number peer_number,
                                       Tox_Message_Type type,
                                       const uint8_t message[], size_t length,
                                       void* opaque);

    static void OnConferenceTitleCb_(Tox* tox,
                                     Tox_Conference_Number conference_number,
                                     Tox_Conference_Peer_Number peer_number,
                                     const uint8_t title[], size_t length,
                                     void* opaque);

    static void OnConferencePeerNameCb_(Tox* tox,
                                        Tox_Conference_Number conference_number,
                                        Tox_Conference_Peer_Number peer_number,
                                        const uint8_t name[], size_t length,
                                        void* opaque);

    static void OnConferencePeerListChangedCb_(
        Tox* tox, Tox_Conference_Number conference_number, void* opaque);

    // Video and audio call-related static callback functions
    static void OnCallCb_(ToxAV* av, Tox_Friend_Number friend_number,
                          bool audio_enabled, bool video_enabled, void* opaque);

    static void OnCallStateCb_(ToxAV* av, Tox_Friend_Number friend_number,
                               uint32_t state, void* opaque);

    static void OnAudioFrameCb_(
        ToxAV* av, Tox_Friend_Number friend_number,
        const int16_t pcm[/*! sample_count * channels */], size_t sample_count,
        uint8_t channels, uint32_t sampling_rate, void* opaque);

    static void OnVideoFrameCb_(
        ToxAV* av, Tox_Friend_Number friend_number, uint16_t width,
        uint16_t height,
        const uint8_t y[/*! max(width, abs(ystride)) * height */],
        const uint8_t u[/*! max(width/2, abs(ustride)) * (height/2) */],
        const uint8_t v[/*! max(width/2, abs(vstride)) * (height/2) */],
        int32_t ystride, int32_t ustride, int32_t vstride, void* opaque);

private:
    // ==================== Callback Function Storage ====================
    // friend-related callbacks
    FriendRequestCb onFriendRequest_{nullptr};
    FriendConnectionStatusCb onFriendConnectionStatus_{nullptr};
    FriendMessageCb onFriendMessage_{nullptr};

    // file transfer-related callbacks
    FileReciveCb onFileRecive_{nullptr};
    FileRecvControlCb onFileRecvControl_{nullptr};
    FileChunkRequestCb onFileChunkRequest_{nullptr};
    FileRecvControlCb onFileChunkRecv_{nullptr};

    // conference-related callbacks
    ConferenceInviteCb onConferenceInvite_{nullptr};
    ConferenceConnectedCb onConferenceConnected_{nullptr};
    ConferenceMessageCb onConferenceMessage_{nullptr};
    ConferenceTitleCb onConferenceTitle_{nullptr};
    ConferencePeerNameCb onConferencePeerName_{nullptr};
    ConferencePeerListChangedCb onConferencePeerListChanged_{nullptr};

    // video and audio related callbacks
    CallCb onCall_{nullptr};
    CallStateCb onCallState_{nullptr};
    AudioFrameCb onAudioFrame_{nullptr};
    VideoFrameCb onVideoFrame_{nullptr};

private:
    Tox* tox_{nullptr};
    ToxAV* av_{nullptr};
};
}  // namespace ToxCore
