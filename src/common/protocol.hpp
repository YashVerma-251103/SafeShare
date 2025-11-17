// ? message types, JSON schemas, constants.

#pragma once
#include <string>

namespace proto
{
    constexpr int DEFAULT_PORT = 55001;
    constexpr int DISCOVERY_PORT = 55000;
    inline const std::string MSG_PERM_REQUEST = "PERM_REQUEST";
    inline const std::string MSG_PERM_RESPONSE = "PERM_RESPONSE";
    inline const std::string MSG_ANNOUNCE = "ANNOUNCE";
    inline const std::string MSG_LIST = "LIST";
}