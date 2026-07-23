// ? listen for announcements and update local cache.

#include <unistd.h>
#include <vector>
#include <chrono>
#include <nlohmann/json.hpp>
#include <sys/socket.h>
#include <arpa/inet.h>

#include "discovery.hpp"
#include "../common/protocol.hpp"

using json = nlohmann::json;

std::vector<DiscoveredPeer> scan_once(int listen_ms)
{
    std::vector<DiscoveredPeer> res;
    int fd = socket(AF_INET, SOCK_DGRAM, 0);
    if (fd < 0)
        return res;
    int opt = 1;
    setsockopt(fd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));
    sockaddr_in addr{};
    addr.sin_family = AF_INET;
    addr.sin_port = htons(proto::DISCOVERY_PORT);
    addr.sin_addr.s_addr = INADDR_ANY;
    if (bind(fd, (sockaddr *)&addr, sizeof(addr)) < 0)
    {
        close(fd);
        return res;
    }
    // set timeout
    timeval tv{};
    tv.tv_sec = listen_ms / 1000;
    tv.tv_usec = (listen_ms % 1000) * 1000;
    setsockopt(fd, SOL_SOCKET, SO_RCVTIMEO, &tv, sizeof(tv));
    char buf[8192];
    sockaddr_in src{};
    socklen_t sl = sizeof(src);
    while (true)
    {
        ssize_t r = recvfrom(fd, buf, sizeof(buf) - 1, 0, (sockaddr *)&src, &sl);
        if (r <= 0)
            break;
        buf[r] = '\0';
        try
        {
            json j = json::parse(std::string(buf));
            if (j.contains("type") && j["type"].get<std::string>() == proto::MSG_ANNOUNCE)
            {
                DiscoveredPeer p;
                p.device_id = j.value("device_id", "");
                p.name = j.value("name", "");
                p.port = j.value("port", proto::DEFAULT_PORT);
                char ipbuf[INET_ADDRSTRLEN];
                inet_ntop(AF_INET, &src.sin_addr, ipbuf, sizeof(ipbuf));
                p.ip = ipbuf;
                res.push_back(p);
            }
        }
        catch (...)
        {
        }
    }
    close(fd);
    return res;
}