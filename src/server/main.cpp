// // ? server entry point (init certs, start services).

// #include "server.hpp"
// #include <iostream>
// #include "../common/protocol.hpp"
// #include "../discovery/discovery.hpp"

// int main(int argc, char **argv)
// {
//     int port = proto::DEFAULT_PORT;
//     std::string shared_dir = "./shared";
//     // if (argc > 1)
//     //     port = std::stoi(argv[1]);
//     // if (argc > 2)
//     //     shared = argv[2];
//     for (int i = 1; i < argc; i++) {
//     std::string arg = argv[i];

//         if (arg == "--shared" && i + 1 < argc) {
//             shared_dir = argv[++i];
//         }
//         else if (arg == "--port" && i + 1 < argc) {
//             try {
//                 port = std::stoi(argv[++i]);
//             } catch (...) {
//                 std::cerr << "Invalid port number.\n";
//                 exit(1);
//             }
//         }
//         else {
//             std::cerr << "Unknown argument: " << arg << "\n";
//         }
//     }

//     // start announcer
//     start_announcer("SafeShare-Device", port);
//     Server s(port, shared);
//     s.run();
//     return 0;
// }
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
