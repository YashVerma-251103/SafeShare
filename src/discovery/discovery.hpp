// ? discovery API (start/stop, callback on new device).

#pragma once
#include <string>
#include <vector>

struct DiscoveredPeer
{
    std::string device_id;
    std::string name;
    std::string ip;
    int port;
};

// ? start the announcer in a detached thread
void start_announcer(const std::string &name, int service_port);

// ? scan once for peers (blocking short time) and return list
std::vector<DiscoveredPeer> scan_once(int listen_ms = 800);