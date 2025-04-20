// client.cpp
#include <iostream>
#include <flatbuffers/flatbuffers.h>
#include <Network.hpp>
#include <server_generated.h>

using asio::ip::tcp;

int main() {
    std::cout.setf(std::ios::unitbuf);

    std::cout << "client\n";
    sleep(2);

    std::cout << "constructing request\n";
    
    flatbuffers::FlatBufferBuilder payload_builder;
    auto username = payload_builder.CreateString("maxortner");
    auto password = payload_builder.CreateString("password");
    auto login = Server::CreateLoginRequest(payload_builder, username, password);
    payload_builder.Finish(login);

    auto response = Network::get_response<Server::LoginResponse>("server", "login", payload_builder);
    std::cout << "Response success: " << ( response->success ? "True" : "False" ) << "\n";

    return 0;
}
