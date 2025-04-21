cmake_minimum_required(VERSION 3.8)

project(server)

set(CMAKE_CXX_STANDARD 20)
find_package(PostgreSQL REQUIRED)

add_executable(server 
    ${CMAKE_SOURCE_DIR}/src/db/Companies.cpp
    ${CMAKE_SOURCE_DIR}/src/db/Prices.cpp

    ${CMAKE_SOURCE_DIR}/src/main.cpp
)

target_link_directories(server 
    PRIVATE
        ${CMAKE_SOURCE_DIR}/external/flatbuffers
        ${CMAKE_SOURCE_DIR}/external/prometheus/lib
)

target_link_libraries(server
    PRIVATE
        flatbuffers
        pthread
        prometheus-cpp-core
        prometheus-cpp-pull
        prometheus-cpp-push
        PostgreSQL::PostgreSQL
)

target_include_directories(server
    PRIVATE
        ${CMAKE_SOURCE_DIR}/external/asio/include
        ${CMAKE_SOURCE_DIR}/external/date/include
        ${CMAKE_SOURCE_DIR}/external/prometheus/core/include
        ${CMAKE_SOURCE_DIR}/external/prometheus/pull/include
        ${CMAKE_SOURCE_DIR}/external/prometheus/push/include
        ${CMAKE_SOURCE_DIR}/external/flatbuffers/include
        ${CMAKE_SOURCE_DIR}/external/networking
        ${CMAKE_SOURCE_DIR}/schemas
        ${CMAKE_SOURCE_DIR}/src
)