#include "Companies.hpp"

#include <cassert>

namespace db
{

int32_t
Companies::get_id(const std::string& company_name) const
{
    PGconn* conn = _db->conn;

    const std::string query = "SELECT id FROM companies WHERE name = $1";
    assert(_db->prepare("get_company_id", query, 1));

    auto response = _db->get_row<int32_t>("get_company_id", { company_name.c_str() });
    if (!response) return -1;

    return std::get<0>(*response);
}

}