// client.cpp
#include <asio.hpp>
#include <iostream>

using asio::ip::tcp;

int main() {
    std::cout << "client\n";
    try {
        asio::io_context io;

        tcp::resolver resolver(io);
        auto endpoints = resolver.resolve("server", "8080");

        tcp::socket socket(io);
        asio::connect(socket, endpoints);

        std::string message = "Hello from client!";
        asio::write(socket, asio::buffer(message));

        std::cout << "Message sent.\n";

    } catch (const std::exception& e) {
        std::cerr << "Client error: " << e.what() << "\n";
    }

    return 0;
}
