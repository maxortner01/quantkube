#pragma once

#include "Companies.hpp"

#include <chrono>

namespace db
{
    using timestamp = std::chrono::system_clock::time_point;

    struct PriceEntry
    {
        timestamp time;
        std::string company_name;
        float price;
    };

    struct Prices
    {   
        Prices(const std::shared_ptr<Database>& database) :
            _db(database),
            companies(database)
        {   }
        
        std::vector<PriceEntry>
        get_prices(timestamp start_time, timestamp end_time, const std::string& company_name) const;

    private:
        std::shared_ptr<Database> _db;
        Companies companies;
    };
}