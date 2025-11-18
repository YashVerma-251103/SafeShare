# Name the output executable "safeshare-node"
add_executable(safeshare-node
    ../main.cpp               # The Unified Entry Point
    ../ui/mini_web.cpp        # The Web Server logic
    server.cpp                # Server logic
    token_manager.cpp
    file_manager.cpp
    chat_manager.cpp
)

# Link the necessary libraries (Discovery, Common, SQLite, Threads)
target_link_libraries(safeshare-node PRIVATE 
    discovery 
    common 
    ${SQLITE3_LIBRARIES} 
    Threads::Threads
)

# Include directories for headers
target_include_directories(safeshare-node PRIVATE 
    ${CMAKE_CURRENT_SOURCE_DIR}/../common
    ${CMAKE_CURRENT_SOURCE_DIR}/../client
    ${CMAKE_CURRENT_SOURCE_DIR}/../ui
    ${SQLITE3_INCLUDE_DIRS}
)