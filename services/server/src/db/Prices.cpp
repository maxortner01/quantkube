#include "Prices.hpp"

#include <date/date.h>

namespace db
{
    
long long to_unix_timestamp(timestamp tp) {
    auto duration = tp.time_since_epoch();
    return std::chrono::duration_cast<std::chrono::seconds>(duration).count();
}

timestamp parse_timestamp(const std::string& timestamp) {
    // Convert the input timestamp "2025-04-21 00:52:44.292436+00" to the format "2025-04-21T00:52:44.292436"
    std::string iso_format = timestamp;
    std::replace(iso_format.begin(), iso_format.end(), ' ', 'T'); // Replace space with 'T'

    // Now parse using the date::parse function (fractional seconds + time zone)
    db::timestamp tp;
    std::istringstream ss(iso_format);
    ss >> date::parse("%FT%T%z", tp);

    if (ss.fail()) {
        throw std::runtime_error("Failed to parse timestamp: " + iso_format);
    }

    return tp;
}

std::vector<PriceEntry>
Prices::get_prices(
    timestamp start_time, 
    timestamp end_time, 
    const std::string& company_name) const
{
    // Convert start and end times to Unix timestamps
    long long start_ts = to_unix_timestamp(start_time);
    long long end_ts = to_unix_timestamp(end_time);

    const auto company_id = companies.get_id(company_name);
    if (company_id <= 0) return {};

    const std::string price_query = R"(
        SELECT time, price
        FROM price_data
        WHERE company_id = $1 AND time >= to_timestamp($2) AND time <= to_timestamp($3)
        ORDER BY time ASC
    )";

    assert(_db->prepare("get_price_data", price_query, 3));
    auto prices = _db->get_rows<std::string, float>("get_price_data", { 
        std::to_string(company_id).c_str(),
        std::to_string(start_ts).c_str(),
        std::to_string(end_ts).c_str()
    });

    if (!prices)
    {
        std::cout << "Error getting prices\n";
        return {};
    }
    
    std::vector<PriceEntry> r;
    r.reserve(prices->size());

    for (const auto& [ time_str, price ] : *prices)
    {
        r.emplace_back(PriceEntry {
            .time         = parse_timestamp(time_str),
            .company_name = company_name,
            .price        = price
        });
    }    

    return r;
}

}