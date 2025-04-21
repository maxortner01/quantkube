#pragma once

#include <tuple>
#include <sstream>
#include <libpq-fe.h>
#include <set>
#include <optional>
#include <vector>
#include <memory>
#include <iostream>
#include <cassert>

namespace db
{
    namespace util
    {

    template<typename T>
    T from_cstr(const char* str);
    
    template<>
    inline int from_cstr<int32_t>(const char* str) {
        return std::stoi(str);
    }
    
    template<>
    inline float from_cstr<float>(const char* str) {
        return std::stof(str);
    }
    
    template<>
    inline std::string from_cstr<std::string>(const char* str) {
        return std::string(str);
    }

    template<typename T>
    T parse_column(PGresult* res, int col_idx) {
        const char* val = PQgetvalue(res, 0, col_idx);
        return from_cstr<T>(val);
    }

    // Helper to apply parse_column to all columns
    template<typename... Ret, std::size_t... I>
    std::tuple<Ret...> parse_row(PGresult* res, int row, std::index_sequence<I...>) {
        return std::make_tuple(parse_column<Ret>(res, I)...);
    }

    }

    struct Database
    {
        PGconn* conn;

        Database(const std::string& host, const std::string& dbname, const std::string& user, const std::string& pass) :
            conn(nullptr)
        {
            std::stringstream ss;
            ss << "host="     << host << " ";
            ss << "dbname="   << dbname << " ";
            ss << "user="     << user << " ";
            ss << "password=" << pass;
            
            conn = PQconnectdb(ss.str().c_str());
            if (PQstatus(conn) != CONNECTION_OK)
            {
                std::cout << "Error connecting to db: " << PQerrorMessage(conn) << "\n";
                PQfinish(conn);
            }
        }

        Database(const Database&) = delete;
        Database(Database&&) = delete;

        ~Database()
        {
            PQfinish(conn);
        }

        bool prepare(const std::string& name, const std::string& query, uint32_t args)
        {
            if (prepared.count(name)) return true; // need to do better

            PGresult* res = PQprepare(conn, name.c_str(), query.c_str(), args, nullptr);

            if (PQresultStatus(res) != PGRES_COMMAND_OK) {
                std::cerr << "Prepare failed: " << PQerrorMessage(conn) << std::endl;
                PQclear(res);
                return false;
            }
    
            prepared.insert(name);
            PQclear(res);
            return true;
        }

        template<typename... Ret>
        std::optional<std::vector<std::tuple<Ret...>>> 
        get_rows(const std::string& name, const std::vector<const char*>& args) 
        {
            assert(prepared.count(name));

            PGresult* res = PQexecPrepared(conn, name.c_str(), args.size(), args.data(), nullptr, nullptr, 0);

            if (PQresultStatus(res) != PGRES_TUPLES_OK) {
                std::cerr << "Query failed: " << PQerrorMessage(conn) << std::endl;
                PQclear(res);
                return {};
            }

            int rows = PQntuples(res);
            std::vector<std::tuple<Ret...>> results;
            results.reserve(rows);

            for (int i = 0; i < rows; ++i)
                results.emplace_back(util::parse_row<Ret...>(res, i, std::index_sequence_for<Ret...>{}));

            PQclear(res);
            return results;
        }

        template<typename... Ret>
        std::optional<std::tuple<Ret...>> get_row(const std::string& name, const std::vector<const char*>& args)
        {
            assert(prepared.count(name));
            auto rows = get_rows<Ret...>(name, args);

            if (!rows || (rows && rows->empty())) return std::nullopt;
            return rows->at(0);
        }

    private:
        std::set<std::string> prepared;
    };
}