// client.cpp
#include <asio.hpp>
#include <iostream>

#include <flatbuffers/flatbuffers.h>

#include <base_generated.h>
#include <server_generated.h>

using asio::ip::tcp;

int main() {
    std::cout << "client\n";
    sleep(2);

    std::cout << "constructing request\n";
    
    flatbuffers::FlatBufferBuilder out_builder, payload_builder;
    auto username = payload_builder.CreateString("maxortner");
    auto password = payload_builder.CreateString("password");
    auto login = Server::CreateLoginRequest(payload_builder, username, password);
    payload_builder.Finish(login);

    auto endpoint = out_builder.CreateString("login");
    auto payload_vec = out_builder.CreateVector(
        payload_builder.GetBufferPointer(),
        payload_builder.GetSize()
    );

    auto request = App::CreateRequest(out_builder, endpoint, payload_vec);
    out_builder.Finish(request);

    try {
        asio::io_context io;

        tcp::resolver resolver(io);
        auto endpoints = resolver.resolve("server", "8080");

        tcp::socket socket(io);
        asio::connect(socket, endpoints);

        asio::write(socket, asio::buffer(
            out_builder.GetBufferPointer(),
            out_builder.GetSize()
        ));

        std::cout << "Message sent.\n";

        char response[1024];
        // Read the response from the server
        std::size_t length = socket.read_some(asio::buffer(response));
        
        auto* res = flatbuffers::GetRoot<App::Response>(response);
        std::cout << "Got " << res->status()->str() << "\n";


    } catch (const std::exception& e) {
        std::cerr << "Client error: " << e.what() << "\n";
    }

    return 0;
}
