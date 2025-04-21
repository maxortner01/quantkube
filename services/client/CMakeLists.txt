cmake_minimum_required(VERSION 3.8)
project(client)

set(CMAKE_CXX_STANDARD 17)

add_executable(client main.cpp)

target_link_directories(client 
    PRIVATE
        ${CMAKE_SOURCE_DIR}/external/flatbuffers
)

target_link_libraries(client
    PRIVATE
        flatbuffers
        pthread
)

target_include_directories(client
    PRIVATE
        ${CMAKE_SOURCE_DIR}/external/asio/include
        ${CMAKE_SOURCE_DIR}/external/date/include
        ${CMAKE_SOURCE_DIR}/external/flatbuffers/include
        ${CMAKE_SOURCE_DIR}/external/networking
        ${CMAKE_SOURCE_DIR}/schemas
)