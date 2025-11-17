// ? server entry point (init certs, start services).

#include "server.hpp"
#include <iostream>
#include "../common/protocol.hpp"
#include "../discovery/discovery.hpp"

int main(int argc, char **argv)
{
    int port = proto::DEFAULT_PORT;
    std::string shared = "./shared";
    if (argc > 1)
        port = std::stoi(argv[1]);
    if (argc > 2)
        shared = argv[2];
    // start announcer
    start_announcer("SafeShare-Device", port);
    Server s(port, shared);
    s.run();
    return 0;
}