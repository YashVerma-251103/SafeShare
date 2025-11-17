#include <sys/socket.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <nlohmann/json.hpp>
#include <string>
#include <iostream>
#include <vector>
#include "../common/framing.hpp"
#include "../discovery/discovery.hpp"
#include <netinet/in.h>
#include <fstream>


using json = nlohmann::json;

#include "protocol.hpp"
// #include "client.hpp"

using json = nlohmann::json;

void send_perm_request(const std::string &ip, int port, const std::string &display, const std::string &reason)
{
    int fd = socket(AF_INET, SOCK_STREAM, 0);
    sockaddr_in addr{};
    addr.sin_family = AF_INET;
    addr.sin_port = htons(port);
    inet_pton(AF_INET, ip.c_str(), &addr.sin_addr);
    if (connect(fd, (sockaddr *)&addr, sizeof(addr)) < 0)
    {
        perror("connect");
        close(fd);
        return;
    }
    json req;
    req["type"] = proto::MSG_PERM_REQUEST;
    req["id"] = display + "-req";
    req["from_device"] = display + "-dev";
    req["display_name"] = display;
    req["reason"] = reason;
    if (!send_frame(fd, req))
    {
        std::cerr << "send failed\n";
        close(fd);
        return;
    }
    // wait for response
    json hdr;
    std::vector<uint8_t> payload;
    if (!read_frame(fd, hdr, payload))
    {
        std::cerr << "no response\n";
        close(fd);
        return;
    }
    std::cerr << "Response: " << hdr.dump() << "\n";
    close(fd);
}

// reexport scan_once to client main


void download_file(const std::string &ip, int port, const std::string &token, const std::string &remote_path, const std::string &local_path)
{
    int fd = socket(AF_INET, SOCK_STREAM, 0);
    sockaddr_in addr{};
    addr.sin_family = AF_INET;
    addr.sin_port = htons(port);
    inet_pton(AF_INET, ip.c_str(), &addr.sin_addr);

    if (connect(fd, (sockaddr *)&addr, sizeof(addr)) < 0) {
        perror("connect");
        return;
    }

    // 1. Send Request
    json req;
    req["type"] = proto::MSG_DOWNLOAD_REQ;
    req["token"] = token;
    req["path"] = remote_path;
    send_frame(fd, req);

    // 2. Loop to receive chunks
    json header;
    std::vector<uint8_t> payload;
    std::ofstream outfile;
    bool receiving = true;
    size_t total_bytes = 0;

    while (receiving && read_frame(fd, header, payload)) {
        std::string type = header.value("type", "");

        if (type == proto::MSG_DOWNLOAD_RESP) {
            std::cout << "Starting download...\n";
            outfile.open(local_path, std::ios::binary);
            if (!outfile.is_open()) {
                std::cerr << "Failed to open local file for writing.\n";
                close(fd);
                return;
            }
        }
        else if (type == proto::MSG_FILE_CHUNK) {
            if (outfile.is_open()) {
                outfile.write((char*)payload.data(), payload.size());
                total_bytes += payload.size();
                std::cout << "\rReceived: " << total_bytes << " bytes" << std::flush;
            }
        }
        else if (type == proto::MSG_TRANSFER_END) {
            std::cout << "\nDownload complete!\n";
            receiving = false;
        }
        else if (type == proto::MSG_ERROR) {
            std::cerr << "\nError from server: " << header.value("message", "unknown") << "\n";
            receiving = false;
        }
    }

    if (outfile.is_open()) outfile.close();
    close(fd);
}