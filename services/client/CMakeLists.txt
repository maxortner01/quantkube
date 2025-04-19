cmake_minimum_required(VERSION 3.8)
project(client)

set(CMAKE_CXX_STANDARD 17)

add_executable(client main.cpp)

target_include_directories(client
    PRIVATE
        ${CMAKE_SOURCE_DIR}/external/asio/asio/include
)