// server.cpp
#include <asio.hpp>
#include <iostream>

using asio::ip::tcp;

int main() {
    try {
        asio::io_context io;

        // Accept connections on port 8080
        tcp::acceptor acceptor(io, tcp::endpoint(tcp::v4(), 8080));
        std::cout << "Server is listening on port 8080...\n";

        tcp::socket socket(io);
        acceptor.accept(socket);

        std::cout << "Client connected.\n";

        char data[1024];
        std::size_t length = socket.read_some(asio::buffer(data));
        std::cout << "Received: " << std::string(data, length) << "\n";

    } catch (const std::exception& e) {
        std::cerr << "Server error: " << e.what() << "\n";
    }

    return 0;
}
