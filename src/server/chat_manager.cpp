// ? handle chat messages and delivery.

#include "chat_manager.hpp"
#include <iostream>
ChatManager::ChatManager() {}
void ChatManager::handleIncoming(const std::string &from, const std::string &body)
{
    std::cout << "[chat] from=" << from << " body=" << body << "\n";
}