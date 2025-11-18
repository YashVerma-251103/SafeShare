#include "server/server.hpp"
#include "discovery/discovery.hpp"
#include <thread>
#include <iostream>
#include <filesystem>

namespace fs = std::filesystem;

void start_web_server();

int main(int argc, char **argv) {
    int port = 55001;
    
    // [FIX 3] Smart Shared Folder Detection
    // 1. Try ./shared (if running from root)
    // 2. Try ../shared (if running from build/)
    // 3. Default to ./shared if neither found (it will be created)
    std::string shared_dir = "./shared";
    
    if (fs::exists("./shared")) {
        shared_dir = "./shared";
    } else if (fs::exists("../shared")) {
        shared_dir = "../shared";
    } else {
        std::cout << "[info] 'shared' folder not found. Creating ./shared\n";
        fs::create_directory("./shared");
    }

    std::cout << "----------------------------------\n";
    std::cout << "SafeShare Node Starting...\n";
    std::cout << "Sharing Folder: " << fs::absolute(shared_dir) << "\n";
    std::cout << "----------------------------------\n";

    // 1. Start Discovery
    start_announcer("SafeShare-WebNode", port);

    // 2. Start File Server
    std::thread server_thread([&](){
        Server s(port, shared_dir);
        s.run();
    });
    server_thread.detach();

    // 3. Start Web Server
    start_web_server(); 

    return 0;
}