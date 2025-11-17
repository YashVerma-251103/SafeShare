// ? UDP broadcast announce.

#include "discovery.hpp"
#include "../common/protocol.hpp"
#include <thread>
#include <chrono>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <unistd.h>
#include <nlohmann/json.hpp>

using json = nlohmann::json;

void start_announcer(const std::string &name, int service_port)
{
    std::thread([name, service_port]{
        int fd = socket(AF_INET, SOCK_DGRAM, 0);
        if (fd < 0) return;
        int broadcast=1;
        setsockopt(fd, SOL_SOCKET, SO_BROADCAST, &broadcast, sizeof(broadcast));
        sockaddr_in addr{};
        addr.sin_family = AF_INET;
        addr.sin_port = htons(proto::DISCOVERY_PORT);
        addr.sin_addr.s_addr = inet_addr("255.255.255.255");

        while (true) {
            json j;
            j["type"] = proto::MSG_ANNOUNCE;
            j["device_id"] = name + "-id"; // simple
            j["name"] = name;
            j["port"] = service_port;
            std::string s = j.dump();
            sendto(fd, s.data(), s.size(), 0, (sockaddr*)&addr, sizeof(addr));
            std::this_thread::sleep_for(std::chrono::milliseconds(800));
            }
        close(fd); 
    }).detach();
}