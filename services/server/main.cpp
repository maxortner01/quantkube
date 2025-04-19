// server.cpp
#include <asio.hpp>
#include <iostream>

#include <flatbuffers/flatbuffers.h>
#include <monster_generated.h>

using asio::ip::tcp;

int main() {
    std::cout.setf(std::ios::unitbuf);
    std::cout << "Starting\n";
    flatbuffers::FlatBufferBuilder builder(1024);

    auto name = builder.CreateString("Orc Man");
    MyGame::MonsterBuilder monsterBuilder(builder);
    monsterBuilder.add_id(1);
    monsterBuilder.add_name(name);
    monsterBuilder.add_hp(300);
    auto orc = monsterBuilder.Finish();

    builder.Finish(orc);

    uint8_t* buf = builder.GetBufferPointer();
    int size = builder.GetSize();

    auto monster = MyGame::GetMonster(buf);
    std::cout << "Monster ID: " << monster->id() << "\n";
    std::cout << "Name: " << monster->name()->str() << "\n";
    std::cout << "HP: " << monster->hp() << "\n";

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
