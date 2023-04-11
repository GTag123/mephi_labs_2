cmake_minimum_required(VERSION 3.16)
project(socket-connect CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

find_package(OpenSSL REQUIRED)
include_directories(${OPENSSL_INCLUDE_DIR})

include(FetchContent)

set(CPR_USE_SYSTEM_CURL ON)
FetchContent_Declare(cpr GIT_REPOSITORY https://github.com/libcpr/cpr.git
        GIT_TAG 0817715923c9705e68994eb52ef9df3f6845beba) # The commit hash for 1.10.x. Replace with the latest from: https://github.com/libcpr/cpr/releases
FetchContent_MakeAvailable(cpr)

add_executable(
        ${PROJECT_NAME}
        main.cpp
        check_peers.h
        peer.h
        torrent_file.h
        tcp_connect.cpp
        tcp_connect.h
        torrent_tracker.cpp
        torrent_tracker.h
        torrent_file.cpp
        bencode.cpp
        bencode.h
        byte_tools.h
        byte_tools.cpp
)
target_link_libraries(${PROJECT_NAME} PUBLIC ${OPENSSL_LIBRARIES} cpr::cpr)
