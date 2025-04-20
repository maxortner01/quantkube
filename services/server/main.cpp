// server.cpp

#include <iostream>
#include <Network.hpp>
#include <server_generated.h>

using asio::ip::tcp;

int main() {
    std::cout.setf(std::ios::unitbuf);

    Network::ServerInstance instance;
    instance.register_endpoint<Server::LoginRequest, Server::LoginResponse>("login", 
        [](Server::LoginResponseT* res,
           const Server::LoginRequestT* req)
        {
            std::cout << "Username: " << req->username << "\n";
            std::cout << "Password: " << req->password << "\n";

            res->success = true;
        }); 

    try {
        instance.run();

    } catch (const std::exception& e) {
        std::cerr << "Server error: " << e.what() << "\n";
    }

    return 0;
}
