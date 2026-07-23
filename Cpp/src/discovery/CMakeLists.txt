add_library(discovery STATIC
discovery.hpp
udp_announcer.cpp
udp_listener.cpp
)
target_link_libraries(discovery PUBLIC common Threads::Threads)