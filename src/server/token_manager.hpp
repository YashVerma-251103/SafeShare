// ? in-memory + SQLite token store (issue/validate/revoke).

#pragma once
#include <string>
#include <unordered_map>
#include <mutex>
#include <chrono>

struct TokenInfo
{
    std::string token;
    std::string device;
    std::string scope;
    uint64_t expires_ms;
    bool persistent;
};

class TokenManager
{
    public:
        TokenManager(const std::string &dbpath);
        ~TokenManager();
        std::string issueToken(const std::string &device, const std::string &scope, int ttl_seconds, bool persistent = false);
        bool validateToken(const std::string &token, const std::string &required_scope);
        void revokeToken(const std::string &token);

    private:
        std::mutex mtx_;
        std::unordered_map<std::string, TokenInfo> tokens_; // in-memory
        // optional: sqlite3* db_; // TODO persist
};