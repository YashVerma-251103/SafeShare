# ? build project targets and options.
cmake_minimum_required(VERSION 3.16)
project(SafeShare LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# options
option(BUILD_CLIENT "Build client" ON)
option(BUILD_SERVER "Build server" ON)
option(BUILD_DISCOVERY "Build discovery" ON)

# find libs
find_package(OpenSSL REQUIRED)
find_package(SQLite3 REQUIRED)
find_package(nlohmann_json REQUIRED)
find_package(Threads REQUIRED)

include_directories(${OPENSSL_INCLUDE_DIR} ${SQLITE3_INCLUDE_DIRS} ${CMAKE_SOURCE_DIR}/third_party)

add_subdirectory(src/common)
add_subdirectory(src/discovery)
add_subdirectory(src/server)
add_subdirectory(src/client)
# optional: add_subdirectory(src/ui)
