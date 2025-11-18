// ? framing helpers (length-prefix read/write).

#pragma once
#include <unistd.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <string>
#include <vector>
#include <nlohmann/json.hpp>

using json = nlohmann::json;

// * read exactly N bytes from fd
static inline bool read_n(int fd, void *buf, size_t n)
{
    size_t off = 0;
    while (off < n)
    {
        ssize_t r = read(fd, (char *)buf + off, n - off);
        if (r <= 0)
            return false;
        off += (size_t)r;
    }
    return true;
}

// * write exactly N bytes
static inline bool write_n(int fd, const void *buf, size_t n)
{
    size_t off = 0;
    while (off < n)
    {
        ssize_t w = write(fd, (const char *)buf + off, n - off);
        if (w <= 0)
            return false;
        off += (size_t)w;
    }
    return true;
}

// * Read a frame: 4-byte BE length (header length), then header JSON string, then if header contains "payload_len" read that many bytes
static inline bool read_frame(int fd, json &header, std::vector<uint8_t> &payload)
{
    uint32_t be_len;
    if (!read_n(fd, &be_len, 4))
        return false;
    uint32_t hdr_len = ntohl(be_len);
    if (hdr_len == 0 || hdr_len > 10 * 1024 * 1024)
        return false; // safety
    std::string hdr(hdr_len, '\0');
    if (!read_n(fd, &hdr[0], hdr_len))
        return false;
    try
    {
        header = json::parse(hdr);
    }
    catch (...)
    {
        return false;
    }
    size_t payload_len = 0;
    if (header.contains("payload_len"))
        payload_len = header["payload_len"].get<size_t>();
    if (payload_len)
    {
        payload.resize(payload_len);
        if (!read_n(fd, payload.data(), payload_len))
            return false;
    }
    else
        payload.clear();
    return true;
}

static inline bool send_frame(int fd, const json &header, const std::vector<uint8_t> *payload = nullptr)
{
    std::string hdr_s = header.dump();
    uint32_t hdr_len = (uint32_t)hdr_s.size();
    uint32_t be = htonl(hdr_len);
    if (!write_n(fd, &be, 4))
        return false;
    if (!write_n(fd, hdr_s.data(), hdr_len))
        return false;
    if (payload && !payload->empty())
    {
        if (!write_n(fd, payload->data(), payload->size()))
            return false;
    }
    return true;
}