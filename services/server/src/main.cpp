#include <iostream>
#include <Server.hpp>
#include <timeseries_generated.h>

#include <date/date.h>

#include <db/Prices.hpp>

// Next up:
//   (/) Have the PostgreSQL database use a volume mounted on the PC so we get persistant data
//   (/) Endpoint latency from server by default
//   (x) Better dependency generation (should do a preimage for each external library then copy in)
//   (x) Use jsonnet everywhere, would be nice for each service to specify its settings then attach prometheus to it as needed, etc.
//   (x) Refactor the networking lib into an actual library
//   (x) Server should wait on healthy db connection
//   (x) Price request for multiple companies
//   (x) Figure out how to determine if a requested time block has missing data in the db

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

            res->company_names.push_back(req->company_name);
            res->company_indices.push_back(0);
            res->data_count.push_back(price_data.size());
            res->prices.reserve(price_data.size());
            res->times.reserve(price_data.size());
            for (const auto& entry : price_data)
            {
                res->times.push_back(util::time_point_to_double(entry.time));
                res->prices.push_back(entry.price);
            }
        }); 

    try {
        instance.run();

    } catch (const std::exception& e) {
        std::cerr << "Server error: " << e.what() << "\n";
    }

    return 0;
}
