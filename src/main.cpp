// src/main.cpp
#include "server/server.hpp"
#include "discovery/discovery.hpp"
#include <thread>
#include <iostream>

// Forward declaration of our web server starter
void start_web_server();

int main(int argc, char **argv) {
    int port = 55001;
    std::string shared_dir = "./shared";
    
    // 1. Start Discovery Announcer (Background)
    start_announcer("SafeShare-WebNode", port);

    // 2. Start File Server (Background)
    std::thread server_thread([&](){
        Server s(port, shared_dir);
        s.run();
    });
    server_thread.detach();

    // 3. Start Web Server (Main Thread - Blocking)
    // This keeps the program alive and serves the UI
    start_web_server(); 

    return 0;
}