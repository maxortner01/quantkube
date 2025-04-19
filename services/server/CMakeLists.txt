cmake_minimum_required(VERSION 3.8)

add_subdirectory(external/flatbuffers)

project(server)

set(CMAKE_CXX_STANDARD 17)

add_executable(server main.cpp)

target_link_libraries(server
    PRIVATE
        FlatBuffers::FlatBuffers
)

target_include_directories(server
    PRIVATE
        ${CMAKE_SOURCE_DIR}/external/asio/asio/include
        ${CMAKE_SOURCE_DIR}/external/flatbuffers/include
        ${CMAKE_SOURCE_DIR}/schemas
)