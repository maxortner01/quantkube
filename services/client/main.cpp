// client.cpp
#include <iostream>
#include <flatbuffers/flatbuffers.h>
#include <Client.hpp>
#include <chrono>

#include <timeseries_generated.h>

using asio::ip::tcp;

int main() {
    std::cout.setf(std::ios::unitbuf);

    std::cout << "client\n";
    sleep(2);

    std::cout << "constructing request\n";
    
    using namespace std::chrono;
    auto now = system_clock::now();
    auto start_time = now - hours(5);
    
    Timeseries::PriceRequestT price_request;
    price_request.company_name = "Acme Corp";
    price_request.start_time   = util::time_point_to_double(start_time);
    price_request.end_time     = util::time_point_to_double(now);

    while (true)
    {
        try{
            auto [code, response] = Network::get_response<Timeseries::PriceResponse, Timeseries::PriceRequest>("server", "get_prices", price_request);
            if (!code)
            {
                std::cout << "Response success\n";
                std::cout << "Got " << response->prices.size() << " price points\n";
                for (int i = 0; i < response->data_count[0]; i++)
                {
                    std::cout << "Company:   " << response->company_names[0] << "\n";
                    std::cout << "Timestamp: " << date::format("%F %T", util::double_to_time_point(response->times[i])) << "\n";
                    std::cout << "Price:     $" << response->prices[i] << "\n";
                }
            }
            else
                std::cout << "Error code received: " << static_cast<int>(code) << "\n";
        } catch (const std::exception& e) {
            std::cerr << "Connection error: " << e.what() << "\n";
        }
    }

    return 0;
}
