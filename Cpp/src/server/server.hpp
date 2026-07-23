// src/server/server.hpp

#pragma once
#include <string>
#include <mutex> 
#include "token_manager.hpp"
#include "file_manager.hpp"
#include "chat_manager.hpp"
#include "../common/framing.hpp"

class Server
{
public:
    Server(int port, const std::string &shared_root);
    void run();

private:
    int port_;
    std::string shared_root_;
    TokenManager tokens_;
    FileManager files_;
    ChatManager chat_;
    std::mutex console_mtx_; // To lock std::cin/cout interactions
    void handle_client(int client_fd, const std::string &peer_ip);
};