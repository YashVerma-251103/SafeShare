// ? message types, JSON schemas, constants.

#pragma once
#include <string>

namespace proto
{
    // Ports
    constexpr int DEFAULT_PORT   = 55001;
    constexpr int DISCOVERY_PORT = 55000;

    // ---- Message type identifiers (strings used in JSON) ----
    inline const std::string MSG_ANNOUNCE      = "ANNOUNCE";
    // Listing files
    inline const std::string MSG_LIST          = "LIST";
    inline const std::string MSG_LIST_RESP     = "LIST_RESP";
    // Permissions
    inline const std::string MSG_PERM_REQUEST  = "PERM_REQUEST";
    inline const std::string MSG_PERM_RESPONSE = "PERM_RESPONSE";
    // Chat message flow
    inline const std::string MSG_CHAT_SEND     = "CHAT_SEND";
    inline const std::string MSG_CHAT_RECV     = "CHAT_RECV";
    // Error handling
    inline const std::string MSG_ERROR         = "ERROR";




    // --- File Transfer Protocol ---
    inline const std::string MSG_DOWNLOAD_REQ  = "DOWNLOAD_REQ";   // Client -> Server
    inline const std::string MSG_DOWNLOAD_RESP = "DOWNLOAD_RESP";  // Server -> Client (Start)
    inline const std::string MSG_FILE_CHUNK    = "FILE_CHUNK";     // Server -> Client (Data)
    inline const std::string MSG_TRANSFER_END  = "TRANSFER_END";   // Server -> Client (Finish)
}
