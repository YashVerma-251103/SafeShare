// ? small helpers (UUID, time, logging macro).

#pragma once
#include <random>
#include <string>
#include <chrono>
#include <sstream>
#include <iomanip>

inline std::string random_hex(size_t len = 32)
{
    static std::random_device rd;
    static std::mt19937_64 eng(rd());
    static std::uniform_int_distribution<unsigned long long> dist(0, std::numeric_limits<unsigned long long>::max());
    std::ostringstream ss;
    while (ss.str().size() < len)
    {
        ss << std::hex << dist(eng);
    }
    auto s = ss.str();
    if (s.size() > len)
        s.resize(len);
    return s;
}

inline uint64_t now_ms()
{
    using namespace std::chrono;
    return (uint64_t)duration_cast<milliseconds>(system_clock::now().time_since_epoch()).count();
}