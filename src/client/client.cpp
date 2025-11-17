#include <sys/socket.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <nlohmann/json.hpp>
#include <string>
#include <iostream>
#include <vector>
#include "../common/framing.hpp"
#include "../discovery/discovery.hpp"

using json = nlohmann::json;

void send_perm_request(const std::string &ip, int port, const std::string &display, const std::string &reason)
{
    int fd = socket(AF_INET, SOCK_STREAM, 0);
    sockaddr_in addr{};
    addr.sin_family = AF_INET;
    addr.sin_port = htons(port);
    inet_pton(AF_INET, ip.c_str(), &addr.sin_addr);
    if (connect(fd, (sockaddr *)&addr, sizeof(addr)) < 0)
    {
        perror("connect");
        close(fd);
        return;
    }
    json req;
    req["type"] = proto::MSG_PERM_REQUEST;
    req["id"] = display + "-req";
    req["from_device"] = display + "-dev";
    req["display_name"] = display;
    req["reason"] = reason;
    if (!send_frame(fd, req))
    {
        std::cerr << "send failed\n";
        close(fd);
        return;
    }
    // wait for response
    json hdr;
    std::vector<uint8_t> payload;
    if (!read_frame(fd, hdr, payload))
    {
        std::cerr << "no response\n";
        close(fd);
        return;
    }
    std::cerr << "Response: " << hdr.dump() << "\n";
    close(fd);
}

// reexport scan_once to client main