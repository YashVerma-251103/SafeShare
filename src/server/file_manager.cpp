// ? list files, open streams, enforce sandbox.

#include "file_manager.hpp"
#include <filesystem>
#include <iostream>

namespace fs = std::filesystem;

FileManager::FileManager(const std::string &shared_root) : root_(shared_root)
{
    if (!fs::exists(root_))
    {
        fs::create_directories(root_);
    }
}

std::vector<std::string> FileManager::list(const std::string &path)
{
    std::vector<std::string> out;
    fs::path p = fs::canonical(fs::path(root_) / path);
    if (p.string().rfind(fs::canonical(root_).string(), 0) != 0)
        return out; // safety
    for (auto &entry : fs::directory_iterator(p))
    {
        out.push_back(entry.path().filename().string());
    }
    return out;
}

bool FileManager::exists(const std::string &path)
{
    fs::path p = fs::canonical(fs::path(root_) / path);
    if (p.string().rfind(fs::canonical(root_).string(), 0) != 0)
        return false;
    return fs::exists(p);
}