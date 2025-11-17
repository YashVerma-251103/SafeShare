// // ? message types, JSON schemas, constants.

// #pragma once
// #include <string>

// namespace proto
// {
//     constexpr int DEFAULT_PORT = 55001;
//     constexpr int DISCOVERY_PORT = 55000;
//     inline const std::string MSG_PERM_REQUEST = "PERM_REQUEST";
//     inline const std::string MSG_PERM_RESPONSE = "PERM_RESPONSE";
//     inline const std::string MSG_ANNOUNCE = "ANNOUNCE";
//     inline const std::string MSG_LIST = "LIST";

// }
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
}
