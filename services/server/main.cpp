// server.cpp
#include <asio.hpp>
#include <iostream>
#include <unordered_map>

#include <flatbuffers/flatbuffers.h>

#include <base_generated.h>
#include <server_generated.h>

using asio::ip::tcp;

struct ServerInstance
{
    std::unordered_map<std::string, std::function<std::shared_ptr<flatbuffers::FlatBufferBuilder>(const uint8_t*)>> endpoints;

    template<typename Req>
    void register_endpoint(const std::string& endpoint, std::function<void(flatbuffers::FlatBufferBuilder&, const Req*)> func)
    {
        endpoints[endpoint] = [func](const uint8_t* data)
        {
            auto builder = std::make_shared<flatbuffers::FlatBufferBuilder>();
            func(
                *builder.get(),
                flatbuffers::GetRoot<Req>(data)
            );

            return builder;
        };
    }

    void process_request(tcp::socket& socket, const uint8_t* data)
    {
        auto request = App::GetRequest(data);
        auto payload_vec = request->payload();
        auto res = endpoints.at(request->endpoint()->str())(payload_vec->Data());

        asio::write(socket, asio::buffer(res->GetBufferPointer(), res->GetSize()));
    }
};

int main() {
    std::cout.setf(std::ios::unitbuf);

    ServerInstance instance;
    instance.register_endpoint<Server::LoginRequest>("login", 
        [](flatbuffers::FlatBufferBuilder& builder,
           const Server::LoginRequest* req)
        {
            std::cout << "Username: " << req->username()->str() << "\n";
            std::cout << "Password: " << req->password()->str() << "\n";

            flatbuffers::FlatBufferBuilder res_builder;
            auto res = Server::CreateLoginResponse(res_builder, true);
            res_builder.Finish(res);

            auto status = builder.CreateString("success");
            auto payload_vec = builder.CreateVector(
                res_builder.GetBufferPointer(),
                res_builder.GetSize()
            );

            auto response = App::CreateResponse(builder, status, payload_vec);
            builder.Finish(response);
        }); 

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
        std::cout << "Received: " << length << " bytes\n";

        instance.process_request(socket, reinterpret_cast<const uint8_t*>(&data[0]));

    } catch (const std::exception& e) {
        std::cerr << "Server error: " << e.what() << "\n";
    }

    return 0;
}
