// ? simple CLI client to discover and send PERM_REQUEST.

#include <iostream>
#include <string>
// #include "client.cpp" // tiny trick to keep both compile units in this skeleton
#include "client.hpp" // tiny trick to keep both compile units in this skeleton

int main(int argc, char **argv)
{
    if (argc < 2)
    {
        std::cout << "Usage: safeshare-client discover | request <ip> <port> <display_name> <reason>\n";
        return 0;
    }
    std::string cmd = argv[1];
    if (cmd == "discover")
    {
        auto peers = scan_once(800);
        std::cout << "Found " << peers.size() << " peers:\n";
        for (auto &p : peers)
        {
            std::cout << p.name << " (" << p.device_id << ") @ " << p.ip << ":" << p.port << "\n";
        }
    }
    else if (cmd == "request")
    {
        if (argc < 6)
        {
            std::cerr << "Usage: safeshare-client request <ip> <port> <display_name> <reason>\n";
            return 1;
        }
        std::string ip = argv[2];
        int port = std::stoi(argv[3]);
        std::string display = argv[4];
        std::string reason = argv[5];
        send_perm_request(ip, port, display, reason);
    }
    return 0;
}