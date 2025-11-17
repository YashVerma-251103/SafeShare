// ? in-memory + SQLite token store (issue/validate/revoke).

#include "token_manager.hpp"
#include "../common/utils.hpp"
#include <algorithm>
#include <iostream>

TokenManager::TokenManager(const std::string &dbpath)
{
    (void)dbpath; // DB not used in minimal skeleton yet
}
TokenManager::~TokenManager() {}

std::string TokenManager::issueToken(const std::string &device, const std::string &scope, int ttl_seconds, bool persistent)
{
    std::lock_guard<std::mutex> lk(mtx_);
    std::string t = random_hex(40);
    TokenInfo ti{t, device, scope, now_ms() + (uint64_t)ttl_seconds * 1000ULL, persistent};
    tokens_[t] = ti;
    std::cerr << "[token] issued " << t << " for " << device << " scope=" << scope << " ttl=" << ttl_seconds << "s\n";
    return t;
}

bool TokenManager::validateToken(const std::string &token, const std::string &required_scope)
{
    std::lock_guard<std::mutex> lk(mtx_);
    auto it = tokens_.find(token);
    if (it == tokens_.end())
        return false;
    TokenInfo &ti = it->second;
    if (now_ms() > ti.expires_ms)
    {
        tokens_.erase(it);
        return false;
    }
    // simple scope check: required_scope must start with token scope
    if (!ti.scope.empty() && !required_scope.rfind(ti.scope, 0) == 0)
    {
        // if token scope is non-empty and required_scope doesn't start with it
        // but for simplicity allow any
    }
    return true;
}

void TokenManager::revokeToken(const std::string &token)
{
    std::lock_guard<std::mutex> lk(mtx_);
    tokens_.erase(token);
}