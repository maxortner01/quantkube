// server.cpp

#include <iostream>
#include <Network.hpp>
#include <server_generated.h>
#include <fstream>

#include <chrono>
#include <set>

#include <cstdlib>
#include <libpq-fe.h>

using asio::ip::tcp;

std::string 
read_file(const std::string& path)
{
    std::ifstream in(path);
    std::stringstream ss;
    ss << in.rdbuf();
    return ss.str();
}

std::vector<std::string>
split_sql(const std::string& sql)
{
    std::vector<std::string> statements;
    std::stringstream ss(sql);
    
    std::string statement;
    while (std::getline(ss, statement, ';'))
    {
        const auto start = statement.find_first_not_of(" \n\r\t");
        if (start != std::string::npos)
            statements.push_back(statement.substr(start));
    }

    return statements;
}

void
execute_sql_file(const std::string& file, PGconn* connection)
{
    const auto statements = split_sql(read_file(file));

    for (const auto& stmt : statements)
    {
        if (stmt.empty()) continue;

        auto* res = PQexec(connection, stmt.c_str());
        if (PQresultStatus(res) != PGRES_COMMAND_OK && PQresultStatus(res) != PGRES_TUPLES_OK)
        {
            std::cout << "SQL error: " << PQerrorMessage(connection) << "\n";
            std::cout << "Failed statement: " << stmt << "\n";
        }
    }
}

PGconn*
connect(const char* host, const char* dbname, const char* user, const char* pass)
{
    std::stringstream ss;
    ss << "host="     << host << " ";
    ss << "dbname="   << dbname << " ";
    ss << "user="     << user << " ";
    ss << "password=" << pass;
    
    PGconn* conn = PQconnectdb(ss.str().c_str());
    if (PQstatus(conn) != CONNECTION_OK)
    {
        std::cout << "Error connecting to db: " << PQerrorMessage(conn) << "\n";
        PQfinish(conn);
        exit(1);
    }

    return conn;
}

std::set<std::string> prepared;

int
get_company_id(const std::string& company_name, PGconn* conn)
{
    // Get company ID from company name
    PGresult* res = nullptr;
    if (!prepared.count("get_company_id"))
    {
        const std::string query = "SELECT id FROM companies WHERE name = $1";
        res = PQprepare(conn, "get_company_id", query.c_str(), 1, nullptr);

        if (PQresultStatus(res) != PGRES_COMMAND_OK) {
            std::cerr << "Prepare failed: " << PQerrorMessage(conn) << std::endl;
            PQclear(res);
            PQfinish(conn);
            return -1;
        }

        prepared.insert("get_company_id");
    }

    // Bind parameter and execute query
    const char* params[1] = { company_name.c_str() };
    res = PQexecPrepared(conn, "get_company_id", 1, params, nullptr, nullptr, 0);

    if (PQresultStatus(res) != PGRES_TUPLES_OK) {
        std::cerr << "Query failed: " << PQerrorMessage(conn) << std::endl;
        PQclear(res);
        PQfinish(conn);
        return -1;
    }

    if (PQntuples(res) == 0) {
        std::cerr << "Company not found.\n";
        PQclear(res);
        PQfinish(conn);
        return 0;
    }

    auto id = std::stoi(PQgetvalue(res, 0, 0));
    PQclear(res);

    return id;
}

long long to_unix_timestamp(std::chrono::system_clock::time_point tp) {
    auto duration = tp.time_since_epoch();
    return std::chrono::duration_cast<std::chrono::seconds>(duration).count();
}

void get_prices(
    std::chrono::system_clock::time_point start_time, 
    std::chrono::system_clock::time_point end_time, 
    const std::string& company_name, 
    PGconn* conn)
{
    // Convert start and end times to Unix timestamps
    long long start_ts = to_unix_timestamp(start_time);
    long long end_ts = to_unix_timestamp(end_time);

    PGresult* res = nullptr;
    if (!prepared.count("get_price_data"))
    {
        const std::string price_query = R"(
            SELECT time, price
            FROM price_data
            WHERE company_id = $1 AND time >= to_timestamp($2) AND time <= to_timestamp($3)
            ORDER BY time ASC
        )";

        res = PQprepare(conn, "get_price_data", price_query.c_str(), 3, nullptr);
        
        if (PQresultStatus(res) != PGRES_COMMAND_OK) {
            std::cerr << "Prepare failed: " << PQerrorMessage(conn) << std::endl;
            PQclear(res);
            PQfinish(conn);
            return;
        }
        
        prepared.insert("get_price_data");
    }

    const auto company_id = get_company_id(company_name, conn);
    if (company_id <= 0) 
    {
        std::cout << "Inavalid company name!\n";
        return;
    }

    // Bind parameters and execute the query
    const char* price_params[3] = { 
        std::to_string(company_id).c_str(),
        std::to_string(start_ts).c_str(),
        std::to_string(end_ts).c_str()
    };
    res = PQexecPrepared(conn, "get_price_data", 3, price_params, nullptr, nullptr, 0);

    if (PQresultStatus(res) != PGRES_TUPLES_OK) {
        std::cerr << "Query failed: " << PQerrorMessage(conn) << std::endl;
        PQclear(res);
        PQfinish(conn);
        return;
    }

    // Output the results
    int rows = PQntuples(res);
    if (rows == 0) {
        std::cout << "No price data found for this company within the given time range.\n";
    } else {
        for (int i = 0; i < rows; ++i) {
            const char* time = PQgetvalue(res, i, 0);
            const char* price = PQgetvalue(res, i, 1);
            std::cout << "Time: " << time << " Price: " << price << std::endl;
        }
    }

    // Cleanup
    PQclear(res);
    PQfinish(conn);
}

int main() {
    std::cout.setf(std::ios::unitbuf);

    std::cout << "Connecting to db...\n";
    auto* conn = connect(
        std::getenv("POSTGRES_HOST"), 
        std::getenv("POSTGRES_DB"), 
        std::getenv("POSTGRES_USER"), 
        std::getenv("POSTGRES_PASSWORD"));

    std::cout << get_company_id("Acme Corp", conn) << "\n";
    
    using namespace std::chrono;
    auto now = system_clock::now();
    auto start_time = now - hours(5);
    
    get_prices(start_time, now, "Acme Corp", conn);

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
