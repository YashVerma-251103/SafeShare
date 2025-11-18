// // ? accept TLS connections, dispatch message handler.

// #include <sys/socket.h>
// #include <arpa/inet.h>
// #include <netinet/in.h>
// #include <unistd.h>
// #include <thread>
// #include <iostream>
// #include <fstream>

// #include <nlohmann/json.hpp>
// using json = nlohmann::json;

// #include "protocol.hpp"
// #include "server.hpp"

// Server::Server(int port, const std::string &shared_root)
//     : port_(port), shared_root_(shared_root), tokens_("./safeshare.db"), files_(shared_root), chat_() {}

// void Server::run()
// {
//     int srv = socket(AF_INET, SOCK_STREAM, 0);
//     if (srv < 0)
//     {
//         perror("socket");
//         return;
//     }
//     int opt = 1;
//     setsockopt(srv, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));
//     sockaddr_in addr{};
//     addr.sin_family = AF_INET;
//     addr.sin_port = htons(port_);
//     addr.sin_addr.s_addr = INADDR_ANY;
//     if (bind(srv, (sockaddr *)&addr, sizeof(addr)) < 0)
//     {
//         perror("bind");
//         close(srv);
//         return;
//     }
//     if (listen(srv, 10) < 0)
//     {
//         perror("listen");
//         close(srv);
//         return;
//     }
//     std::cerr << "Server listening on port " << port_ << "\n";
//     while (true)
//     {
//         sockaddr_in peer{};
//         socklen_t plen = sizeof(peer);
//         int client = accept(srv, (sockaddr *)&peer, &plen);
//         if (client < 0)
//         {
//             perror("accept");
//             continue;
//         }
//         char ipbuf[INET_ADDRSTRLEN];
//         inet_ntop(AF_INET, &peer.sin_addr, ipbuf, sizeof(ipbuf));
//         std::string peer_ip(ipbuf);
//         std::thread(&Server::handle_client, this, client, peer_ip).detach();
//     }
// }

// void Server::handle_client(int client_fd, const std::string &peer_ip)
// {
//     json header;
//     std::vector<uint8_t> payload;
//     while (read_frame(client_fd, header, payload))
//     {
//         std::string type = header.value("type", "");
//         if (type == proto::MSG_PERM_REQUEST)
//         {
//             std::string from = header.value("from_device", "unknown");
//             std::string display = header.value("display_name", "");
//             std::string reason = header.value("reason", "");
//             std::cerr << "[perm] request from=" << from << " name=" << display << " reason=" << reason << " ip=" << peer_ip << "\n";
//             // prompt owner for consent on console
//             std::cout << "Accept share request from '" << display << "' (device=" << from << ") ? (y/n): ";
//             std::cout.flush();
//             char ans = 'n';
//             std::cin >> ans;
//             json resp;
//             resp["type"] = proto::MSG_PERM_RESPONSE;
//             resp["id"] = header.value("id", "");
//             if (ans == 'y' || ans == 'Y')
//             {
//                 std::string token = tokens_.issueToken(from, "/", 900, false);
//                 resp["status"] = "ACCEPT";
//                 resp["token"] = token;
//                 resp["scope"] = std::vector<std::string>{"/"};
//                 resp["ttl"] = 900;
//             }
//             else
//             {
//                 resp["status"] = "DENY";
//             }
//             send_frame(client_fd, resp, nullptr);
//         }
//         else if (type == proto::MSG_LIST)
//         {
//             std::string token = header.value("token", "");
//             std::string path = header.value("path", ".");
//             json resp;
//             resp["type"] = proto::MSG_LIST_RESP;
//             resp["id"] = header.value("id", "");
//             if (!tokens_.validateToken(token, path))
//             {
//                 resp["status"] = "ERROR";
//                 resp["error_code"] = 401;
//                 resp["message"] = "UNAUTHORIZED";
//                 send_frame(client_fd, resp);
//             }
//             else
//             {
//                 auto entries = files_.list(path);
//                 resp["status"] = "OK";
//                 resp["entries"] = entries;
//                 send_frame(client_fd, resp);
//             }
//         }
//         else if (type == proto::MSG_CHAT_SEND)
//         {
//             std::string token = header.value("token", "");
//             if (!tokens_.validateToken(token, "/"))
//             {
//                 json e;
//                 e["type"] = "ERROR";
//                 e["code"] = 401;
//                 e["message"] = "UNAUTHORIZED";
//                 send_frame(client_fd, e);
//                 continue;
//             }
//             std::string from = header.value("from", "?");
//             std::string body = header.value("body", "");
//             chat_.handleIncoming(from, body);
//             json ack;
//             ack["type"] = "CHAT_ACK";
//             ack["id"] = header.value("id", "");
//             ack["status"] = "DELIVERED";
//             send_frame(client_fd, ack);
//         }
//         else if (type == proto::MSG_DOWNLOAD_REQ)
//         {
//             std::string token = header.value("token", "");
//             std::string filename = header.value("path", "");

//             // 1. Validate Token
//             if (!tokens_.validateToken(token, filename))
//             {
//                 json e;
//                 e["type"] = proto::MSG_ERROR;
//                 e["message"] = "UNAUTHORIZED";
//                 send_frame(client_fd, e);
//                 continue;
//             }

//             // 2. Check File Existence
//             if (!files_.exists(filename))
//             {
//                 json e;
//                 e["type"] = proto::MSG_ERROR;
//                 e["message"] = "NOT_FOUND";
//                 send_frame(client_fd, e);
//                 continue;
//             }

//             // 3. Start Transfer
//             // Construct full path (safety checked by FileManager::exists previously)
//             // Note: You might want to expose a "getFullPath" in FileManager to be cleaner
//             std::string full_path = shared_root_ + "/" + filename;
//             std::ifstream file(full_path, std::ios::binary);

//             if (!file.is_open())
//             {
//                 json e;
//                 e["type"] = proto::MSG_ERROR;
//                 e["message"] = "OPEN_FAILED";
//                 send_frame(client_fd, e);
//                 continue;
//             }

//             // Send Start Response
//             json start_resp;
//             start_resp["type"] = proto::MSG_DOWNLOAD_RESP;
//             start_resp["filename"] = filename;
//             send_frame(client_fd, start_resp);

//             // 4. Stream Chunks
//             std::vector<uint8_t> buffer(64 * 1024); // 64KB chunks
//             while (file.read((char *)buffer.data(), buffer.size()) || file.gcount() > 0)
//             {
//                 json chunk_hdr;
//                 chunk_hdr["type"] = proto::MSG_FILE_CHUNK;
//                 chunk_hdr["payload_len"] = file.gcount();

//                 // Resize buffer to actual bytes read for the last chunk
//                 std::vector<uint8_t> chunk_data(buffer.begin(), buffer.begin() + file.gcount());

//                 send_frame(client_fd, chunk_hdr, &chunk_data);
//             }

//             // 5. End Transfer
//             json end_resp;
//             end_resp["type"] = proto::MSG_TRANSFER_END;
//             send_frame(client_fd, end_resp);
//             std::cout << "[server] Sent file: " << filename << "\n";
//         }
//         else
//         {
//             json e;
//             e["type"] = "ERROR";
//             e["message"] = "UNKNOWN MSG";
//             send_frame(client_fd, e);
//         }
//     }
//     close(client_fd);
//     std::cerr << "Client " << peer_ip << " disconnected\n";
// }



// src/server/server.cpp

#include <sys/socket.h>
#include <arpa/inet.h>
#include <netinet/in.h>
#include <unistd.h>
#include <thread>
#include <iostream>
#include <fstream>

#include <nlohmann/json.hpp>
using json = nlohmann::json;

#include "protocol.hpp"
#include "server.hpp"

Server::Server(int port, const std::string &shared_root)
    : port_(port), shared_root_(shared_root), tokens_("./safeshare.db"), files_(shared_root), chat_() {}

void Server::run()
{
    int srv = socket(AF_INET, SOCK_STREAM, 0);
    if (srv < 0) { perror("socket"); return; }
    int opt = 1;
    setsockopt(srv, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));
    sockaddr_in addr{};
    addr.sin_family = AF_INET;
    addr.sin_port = htons(port_);
    addr.sin_addr.s_addr = INADDR_ANY;
    if (bind(srv, (sockaddr *)&addr, sizeof(addr)) < 0) { perror("bind"); close(srv); return; }
    if (listen(srv, 10) < 0) { perror("listen"); close(srv); return; }
    std::cerr << "Server listening on port " << port_ << "\n";
    while (true)
    {
        sockaddr_in peer{};
        socklen_t plen = sizeof(peer);
        int client = accept(srv, (sockaddr *)&peer, &plen);
        if (client < 0) { perror("accept"); continue; }
        char ipbuf[INET_ADDRSTRLEN];
        inet_ntop(AF_INET, &peer.sin_addr, ipbuf, sizeof(ipbuf));
        std::string peer_ip(ipbuf);
        std::thread(&Server::handle_client, this, client, peer_ip).detach();
    }
}

void Server::handle_client(int client_fd, const std::string &peer_ip)
{
    json header;
    std::vector<uint8_t> payload;
    while (read_frame(client_fd, header, payload))
    {
        std::string type = header.value("type", "");

        // --- 1. PERMISSION REQUEST (Pairing) ---
        if (type == proto::MSG_PERM_REQUEST)
        {
            std::string from = header.value("from_device", "unknown");
            std::string display = header.value("display_name", "");
            std::string reason = header.value("reason", "");
            
            std::cout << "\n[!] Connection Request from " << display << " (" << peer_ip << ")\n";
            std::cout << "    Reason: " << reason << "\n";
            
            char ans = 'n';
            {
                // Lock console for interactive prompt
                std::lock_guard<std::mutex> lock(console_mtx_);
                std::cout << "Accept pairing? (y/n): ";
                std::cout.flush();
                std::cin >> ans;
            }

            json resp;
            resp["type"] = proto::MSG_PERM_RESPONSE;
            resp["id"] = header.value("id", "");
            
            if (ans == 'y' || ans == 'Y')
            {
                // Issue token bound to IP
                std::string token = tokens_.issueToken(from, peer_ip, 900, false);
                resp["status"] = "ACCEPT";
                resp["token"] = token;
                resp["ttl"] = 900;
            }
            else
            {
                resp["status"] = "DENY";
            }
            send_frame(client_fd, resp, nullptr);
        }
        
        // --- 2. LIST FILES ---
        else if (type == proto::MSG_LIST)
        {
            std::string token = header.value("token", "");
            std::string path = header.value("path", ".");
            json resp;
            resp["type"] = proto::MSG_LIST_RESP;
            resp["id"] = header.value("id", "");

            // Validate Token + IP
            if (!tokens_.validateToken(token, peer_ip))
            {
                resp["status"] = "ERROR";
                resp["message"] = "UNAUTHORIZED OR INVALID SESSION";
                send_frame(client_fd, resp);
            }
            else
            {
                auto entries = files_.list(path);
                resp["status"] = "OK";
                resp["entries"] = entries;
                send_frame(client_fd, resp);
            }
        }

        // --- 3. DOWNLOAD REQUEST (Interactive Approval) ---
        else if (type == proto::MSG_DOWNLOAD_REQ)
        {
            std::string token = header.value("token", "");
            std::string filename = header.value("path", "");

            // A. Validate Session first
            if (!tokens_.validateToken(token, peer_ip))
            {
                json e; e["type"] = proto::MSG_ERROR; e["message"] = "UNAUTHORIZED";
                send_frame(client_fd, e);
                continue;
            }

            // B. Check File
            if (!files_.exists(filename))
            {
                json e; e["type"] = proto::MSG_ERROR; e["message"] = "NOT_FOUND";
                send_frame(client_fd, e);
                continue;
            }

            // C. INTERACTIVE APPROVAL
            char ans = 'n';
            {
                std::lock_guard<std::mutex> lock(console_mtx_);
                std::cout << "\n[!] Download Request from IP: " << peer_ip << "\n";
                std::cout << "    File: " << filename << "\n";
                std::cout << "Allow this transfer? (y/n): ";
                std::cout.flush();
                std::cin >> ans;
            }

            if (ans != 'y' && ans != 'Y') {
                std::cout << "[server] Denied transfer of " << filename << "\n";
                json e; e["type"] = proto::MSG_ERROR; e["message"] = "ACCESS_DENIED_BY_HOST";
                send_frame(client_fd, e);
                continue;
            }

            std::cout << "[server] Starting transfer: " << filename << "\n";

            // D. Transfer Logic
            std::string full_path = shared_root_ + "/" + filename;
            std::ifstream file(full_path, std::ios::binary);
            if (!file.is_open()) {
                json e; e["type"] = proto::MSG_ERROR; e["message"] = "OPEN_FAILED";
                send_frame(client_fd, e);
                continue;
            }

            json start_resp;
            start_resp["type"] = proto::MSG_DOWNLOAD_RESP;
            start_resp["filename"] = filename;
            send_frame(client_fd, start_resp);

            std::vector<uint8_t> buffer(64 * 1024);
            while (file.read((char *)buffer.data(), buffer.size()) || file.gcount() > 0)
            {
                json chunk_hdr;
                chunk_hdr["type"] = proto::MSG_FILE_CHUNK;
                chunk_hdr["payload_len"] = file.gcount();
                std::vector<uint8_t> chunk_data(buffer.begin(), buffer.begin() + file.gcount());
                send_frame(client_fd, chunk_hdr, &chunk_data);
            }

            json end_resp;
            end_resp["type"] = proto::MSG_TRANSFER_END;
            send_frame(client_fd, end_resp);
            std::cout << "[server] Finished transfer: " << filename << "\n";
        }
        // --- CHAT (Optional, simplified) ---
        else if (type == proto::MSG_CHAT_SEND)
        {
             std::string token = header.value("token", "");
             if (!tokens_.validateToken(token, peer_ip)) { /* Error */ continue; }
             // ... existing chat logic ...
        }
        else
        {
            json e; e["type"] = "ERROR"; e["message"] = "UNKNOWN MSG";
            send_frame(client_fd, e);
        }
    }
    close(client_fd);
}