// #include <sys/socket.h>
// #include <arpa/inet.h>
// #include <unistd.h>
// #include <nlohmann/json.hpp>
// #include <string>
// #include <iostream>
// #include <vector>
// #include "../common/framing.hpp"
// #include "../discovery/discovery.hpp"
// #include <netinet/in.h>
// #include <fstream>


// using json = nlohmann::json;

// #include "protocol.hpp"
// // #include "client.hpp"

// using json = nlohmann::json;

// void send_perm_request(const std::string &ip, int port, const std::string &display, const std::string &reason)
// {
//     int fd = socket(AF_INET, SOCK_STREAM, 0);
//     sockaddr_in addr{};
//     addr.sin_family = AF_INET;
//     addr.sin_port = htons(port);
//     inet_pton(AF_INET, ip.c_str(), &addr.sin_addr);
//     if (connect(fd, (sockaddr *)&addr, sizeof(addr)) < 0)
//     {
//         perror("connect");
//         close(fd);
//         return;
//     }
//     json req;
//     req["type"] = proto::MSG_PERM_REQUEST;
//     req["id"] = display + "-req";
//     req["from_device"] = display + "-dev";
//     req["display_name"] = display;
//     req["reason"] = reason;
//     if (!send_frame(fd, req))
//     {
//         std::cerr << "send failed\n";
//         close(fd);
//         return;
//     }
//     // wait for response
//     json hdr;
//     std::vector<uint8_t> payload;
//     if (!read_frame(fd, hdr, payload))
//     {
//         std::cerr << "no response\n";
//         close(fd);
//         return;
//     }
//     std::cerr << "Response: " << hdr.dump() << "\n";
//     close(fd);
// }
#pragma once
#include <sys/socket.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <nlohmann/json.hpp>
#include <string>
#include <iostream>
#include <vector>
#include <fstream>
#include <filesystem> 
#include "../common/framing.hpp"
#include "../discovery/discovery.hpp"
#include "../common/protocol.hpp"

using json = nlohmann::json;
namespace fs = std::filesystem;

// 1. Request Permission (Returns Token)
inline std::string send_perm_request(const std::string &ip, int port, const std::string &display, const std::string &reason)
{
    int fd = socket(AF_INET, SOCK_STREAM, 0);
    sockaddr_in addr{};
    addr.sin_family = AF_INET;
    addr.sin_port = htons(port);
    inet_pton(AF_INET, ip.c_str(), &addr.sin_addr);
    
    if (connect(fd, (sockaddr *)&addr, sizeof(addr)) < 0) return "";

    json req;
    req["type"] = proto::MSG_PERM_REQUEST;
    req["id"] = display + "-req";
    req["from_device"] = display + "-dev";
    req["display_name"] = display;
    req["reason"] = reason;

    if (!send_frame(fd, req)) { close(fd); return ""; }

    json hdr;
    std::vector<uint8_t> payload;
    if (!read_frame(fd, hdr, payload)) { close(fd); return ""; }
    close(fd);

    if (hdr.value("status", "") == "ACCEPT") return hdr.value("token", "");
    return "";
}

// 2. List Remote Files
inline std::vector<std::string> list_remote_files(const std::string &ip, int port, const std::string &token)
{
    std::vector<std::string> files;
    int fd = socket(AF_INET, SOCK_STREAM, 0);
    sockaddr_in addr{};
    addr.sin_family = AF_INET; addr.sin_port = htons(port);
    inet_pton(AF_INET, ip.c_str(), &addr.sin_addr);

    if (connect(fd, (sockaddr *)&addr, sizeof(addr)) < 0) return files;

    json req;
    req["type"] = proto::MSG_LIST;
    req["token"] = token;
    req["path"] = ".";
    send_frame(fd, req);

    json resp;
    std::vector<uint8_t> payload;
    if (read_frame(fd, resp, payload)) {
        if (resp["status"] == "OK" && resp.contains("entries")) {
            for (auto &f : resp["entries"]) {
                files.push_back(f.get<std::string>());
            }
        }
    }
    close(fd);
    return files;
}

// 3. Download File (FIXED: Added local_path argument back)
inline std::string download_file(const std::string &ip, int port, const std::string &token, const std::string &remote_path, const std::string &local_path)
{
    int fd = socket(AF_INET, SOCK_STREAM, 0);
    sockaddr_in addr{};
    addr.sin_family = AF_INET; addr.sin_port = htons(port);
    inet_pton(AF_INET, ip.c_str(), &addr.sin_addr);

    if (connect(fd, (sockaddr *)&addr, sizeof(addr)) < 0) return "Connection Failed";

    json req;
    req["type"] = proto::MSG_DOWNLOAD_REQ;
    req["token"] = token;
    req["path"] = remote_path;
    send_frame(fd, req);

    // Note: We assume the caller ensures the directory exists
    std::ofstream outfile(local_path, std::ios::binary);
    if (!outfile.is_open()) { close(fd); return "File Write Error: " + local_path; }

    json header;
    std::vector<uint8_t> payload;
    bool receiving = true;
    std::string result = "Download Complete: " + local_path;

    while (receiving && read_frame(fd, header, payload)) {
        std::string type = header.value("type", "");
        if (type == proto::MSG_FILE_CHUNK) {
            outfile.write((char*)payload.data(), payload.size());
        }
        else if (type == proto::MSG_TRANSFER_END) {
            receiving = false;
        }
        else if (type == proto::MSG_ERROR) {
            result = "Error: " + header.value("message", "Unknown");
            receiving = false;
            outfile.close();
            std::remove(local_path.c_str()); // delete partial
        }
    }
    if (outfile.is_open()) outfile.close();
    close(fd);
    return result;
}