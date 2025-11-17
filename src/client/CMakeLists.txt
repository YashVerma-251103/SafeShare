add_executable(safeshare-client
main.cpp
client.cpp
)
target_link_libraries(safeshare-client PRIVATE discovery common Threads::Threads)