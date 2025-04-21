#include <iostream>
#include <Network.hpp>
#include <timeseries_generated.h>

#include <prometheus/exposer.h>
#include <prometheus/registry.h>
#include <prometheus/counter.h>

#include <date/date.h>

#include <db/Prices.hpp>

// Next up:
//   1. Have the PostgreSQL database use a volume mounted on the PC so we get persistant data


int main() {
    std::cout.setf(std::ios::unitbuf);

    prometheus::Exposer exposer{"0.0.0.0:8080"};
    auto registry = std::make_shared<prometheus::Registry>();

    // Add a counter family to the registry
    auto& packet_counter_family = prometheus::BuildCounter()
                                      .Name("packets_processed_total")
                                      .Help("Total packets processed")
                                      .Register(*registry);

    // Add a label to the counter
    auto& packet_counter = packet_counter_family.Add({{"type", "tcp"}});

    // Register the registry to be exposed
    exposer.RegisterCollectable(registry);

    auto database = std::make_shared<db::Database>(
        std::getenv("POSTGRES_HOST"), 
        std::getenv("POSTGRES_DB"), 
        std::getenv("POSTGRES_USER"), 
        std::getenv("POSTGRES_PASSWORD"));

    db::Companies companies(database);
    db::Prices prices(database);

    uint32_t test = 0;

    Network::ServerInstance instance;
    instance.register_endpoint<Timeseries::PriceRequest, Timeseries::PriceResponse>("get_prices", 
        [&](Timeseries::PriceResponseT* res,
           const Timeseries::PriceRequestT* req)
        {
            packet_counter.Increment();
            test++;
            std::cout << test << "\n";

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
