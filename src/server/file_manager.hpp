// ? list files, open streams, enforce sandbox.

#pragma once
#include <string>
#include <vector>

class FileManager
{
public:
    FileManager(const std::string &shared_root);
    std::vector<std::string> list(const std::string &path);
    bool exists(const std::string &path);

private:
    std::string root_;
};