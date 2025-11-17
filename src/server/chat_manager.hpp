// ? handle chat messages and delivery.

#pragma once
#include <string>

class ChatManager
{
public:
    ChatManager();
    void handleIncoming(const std::string &from, const std::string &body);
};