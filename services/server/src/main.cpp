#include <iostream>
#include <Server.hpp>
#include <timeseries_generated.h>

#include <date/date.h>

#include <db/Prices.hpp>

// Next up:
//   1. Have the PostgreSQL database use a volume mounted on the PC so we get persistant data
//   2. Endpoint latency from server by default

int main() {
    std::cout.setf(std::ios::unitbuf);
    
    Network::ServerInstance instance;

    auto database = std::make_shared<db::Database>(
        std::getenv("POSTGRES_HOST"), 
        std::getenv("POSTGRES_DB"), 
        std::getenv("POSTGRES_USER"), 
        std::getenv("POSTGRES_PASSWORD"));

    db::Companies companies(database);
    db::Prices prices(database);

    instance.register_endpoint<Timeseries::PriceRequest, Timeseries::PriceResponse>("get_prices", 
        [&](Timeseries::PriceResponseT* res,
           const Timeseries::PriceRequestT* req)
        {
            const auto start_time = util::double_to_time_point(req->start_time);
            const auto end_time   = util::double_to_time_point(req->end_time);
            
            //std::cout << "Getting " << req->company_name << " between " << date::format("%F %T", start_time) << " -> " << date::format("%F %T", end_time) << "\n";
            const auto price_data = prices.get_prices(start_time, end_time, req->company_name);

            res->prices.reserve(price_data.size());
            for (const auto& entry : price_data)
            {
                auto e = std::make_unique<Timeseries::PriceEntryT>();
                
                e->price        = entry.price;
                e->company_name = entry.company_name;
                e->time         = util::time_point_to_double( entry.time );
            
                res->prices.push_back(std::move(e));
            }
        }); 

    try {
        instance.run();

    } catch (const std::exception& e) {
        std::cerr << "Server error: " << e.what() << "\n";
    }

    return 0;
}
