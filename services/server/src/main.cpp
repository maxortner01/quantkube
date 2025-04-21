#include <iostream>
#include <Network.hpp>
#include <server_generated.h>

#include <date/date.h>

#include <db/Prices.hpp>

int main() {
    std::cout.setf(std::ios::unitbuf);

    auto database = std::make_shared<db::Database>(
        std::getenv("POSTGRES_HOST"), 
        std::getenv("POSTGRES_DB"), 
        std::getenv("POSTGRES_USER"), 
        std::getenv("POSTGRES_PASSWORD"));

    db::Prices prices(database);
    
    using namespace std::chrono;
    auto now = system_clock::now();
    auto start_time = now - hours(5);

    auto price_data = prices.get_prices(start_time, now, "Acme Corp");

    std::cout << "Got " << price_data.size() << " price points\n";
    for (const auto& price : price_data)
        std::cout << date::format("%Y-%m-%d %H:%M:%S", price.time) << ": $" << price.price << "\n";

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
