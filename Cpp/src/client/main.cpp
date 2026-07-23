#include <iostream>
#include <string>
#include "client.hpp"

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
    else if (cmd == "download")
    {
        if (argc < 7)
        {
            std::cerr << "Usage: safeshare-client download <ip> <port> <token> <remote_path> <local_path>\n";
            return 1;
        }
        std::string ip = argv[2];
        int port = std::stoi(argv[3]);
        std::string token = argv[4];
        std::string rpath = argv[5];
        std::string lpath = argv[6];
        download_file(ip, port, token, rpath, lpath);
    }
    else if (cmd == "list")
    {
        if (argc < 5)
        {
            std::cerr << "Usage: safeshare-client list <ip> <port> <token>\n";
            return 1;
        }
        std::string ip = argv[2];
        int port = std::stoi(argv[3]);
        std::string token = argv[4];

        int fd = socket(AF_INET, SOCK_STREAM, 0);
        sockaddr_in addr{};
        addr.sin_family = AF_INET;
        addr.sin_port = htons(port);
        inet_pton(AF_INET, ip.c_str(), &addr.sin_addr);

        if (connect(fd, (sockaddr *)&addr, sizeof(addr)) < 0)
        {
            perror("connect");
            return 1;
        }

        json req;
        req["type"] = proto::MSG_LIST;
        req["token"] = token;
        req["path"] = ".";
        send_frame(fd, req);

        json resp;
        std::vector<uint8_t> payload;
        if (read_frame(fd, resp, payload))
        {
            if (resp["status"] == "OK")
            {
                std::cout << "--- Remote Files ---\n";
                for (auto &f : resp["entries"])
                {
                    std::cout << " - " << f.get<std::string>() << "\n";
                }
                std::cout << "--------------------\n";
            }
            else
            {
                std::cout << "Error: " << resp.value("message", "Unknown") << "\n";
            }
        }
        close(fd);
    }
    return 0;
}