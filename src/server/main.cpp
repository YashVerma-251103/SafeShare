#include "server.hpp"
#include <iostream>
#include "../common/protocol.hpp"
#include "../discovery/discovery.hpp"

int main(int argc, char **argv)
{
    int port = proto::DEFAULT_PORT;
    std::string shared_dir = "./shared";

    // Argument parsing
    for (int i = 1; i < argc; i++) {
        std::string arg = argv[i];

        if (arg == "--shared" && i + 1 < argc) {
            shared_dir = argv[++i];
        }
        else if (arg == "--port" && i + 1 < argc) {
            try {
                port = std::stoi(argv[++i]);
            } catch (...) {
                std::cerr << "Invalid port number.\n";
                return 1;
            }
        }
        else {
            std::cerr << "Unknown argument: " << arg << "\n";
        }
    }

    // Start Announcer
    start_announcer("SafeShare-Device", port);

    // FIX: use shared_dir, not "shared"
    Server s(port, shared_dir);
    s.run();

    return 0;
}
