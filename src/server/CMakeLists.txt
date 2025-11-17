add_executable(safeshare-server
main.cpp
server.cpp
token_manager.cpp
file_manager.cpp
chat_manager.cpp
)
target_link_libraries(safeshare-server PRIVATE discovery common ${SQLITE3_LIBRARIES} Threads::Threads)
include_directories(${SQLITE3_INCLUDE_DIRS})