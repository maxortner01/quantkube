#pragma once

#include "Database.hpp"

namespace db
{
    struct Companies
    {   
        Companies(const std::shared_ptr<Database>& database) :
            _db(database)
        {   }

        int32_t get_id(const std::string& company_name) const;
        
    private:
        std::shared_ptr<Database> _db;
    };
}