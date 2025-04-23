#pragma once

#include <date/date.h>

#include <asio.hpp>
#include <iostream>
#include <unordered_map>

#include <flatbuffers/flatbuffers.h>

#include <prometheus/exposer.h>
#include <prometheus/registry.h>
#include <prometheus/counter.h>
#include <prometheus/gauge.h>

#include <thread>

#include <base_generated.h>

using asio::ip::tcp;

namespace util
{
    double time_point_to_double(std::chrono::system_clock::time_point tp) {
        using namespace std::chrono;
    
        auto duration = tp.time_since_epoch();  // duration since 1970
        auto micros = duration_cast<microseconds>(duration).count(); // total microseconds
    
        return micros / 1'000'000.0;  // Convert to seconds with fractions
    }

    std::chrono::system_clock::time_point double_to_time_point(double unix_ts) {
        using namespace std::chrono;
    
        auto micros = static_cast<int64_t>(unix_ts * 1'000'000);
        return system_clock::time_point{microseconds{micros}};
    }
}

namespace Network
{
    
struct ServerInstance;

struct Request
{
    std::chrono::time_point<std::chrono::high_resolution_clock> request_time;
    std::shared_ptr<tcp::socket> socket;
    std::vector<uint8_t> buffer;
    std::function<void(std::shared_ptr<Request>)> process;

    Request(const uint8_t* data, size_t length, const std::shared_ptr<tcp::socket>& s, ServerInstance* server_instance);
};

template<typename T>
std::shared_ptr<T> SharedFromReference(T& ref) {
    return std::shared_ptr<T>(&ref, [](T*) {});
}

struct ServerInstance
{
    std::unordered_map<std::string, std::function<void(flatbuffers::FlatBufferBuilder&, const uint8_t*)>> endpoints;

    const int thread_count = 5;
    bool running = true;
    std::mutex req_lock, listen_lock;
    std::vector<std::function<void()>> listeners;
    std::vector<std::shared_ptr<Request>> requests;
    std::vector<std::thread> threads;
    
    prometheus::Exposer exposer;
    std::shared_ptr<prometheus::Registry> registry;
    std::shared_ptr<prometheus::Family<prometheus::Gauge>> latency_family;
    std::unordered_map<std::string, prometheus::Gauge*> latency_metrics;

    ServerInstance() :
        exposer{"0.0.0.0:8080"},
        registry(std::make_shared<prometheus::Registry>())
    {
        exposer.RegisterCollectable(registry);

        latency_family = SharedFromReference(
            prometheus::BuildGauge()
                .Name("function_latency_seconds")
                .Help("Latency of endpoints")
                .Labels({})
                .Register(*registry)
        );

        // Set up the threads
        for (int i = 0; i < thread_count; i++)
        {
            threads.push_back(std::thread([this]()
                {
                    while (running)
                    {
                        // First check listeners
                        listen_lock.lock();
                        if (!listeners.empty())
                        {
                            auto f = listeners.back();
                            listeners.pop_back();
                            listen_lock.unlock();
                            f();
                        }
                        else
                            listen_lock.unlock();

                        // Then check requests
                        req_lock.lock();
                        if (!requests.empty())
                        {
                            auto r = requests.back();
                            requests.pop_back();
                            req_lock.unlock();
                            r->process(r);
                        }
                        else
                            req_lock.unlock();
                    }
                }));
        }
    }

    ~ServerInstance()
    {
        running = false;
        for (auto& t : threads)
            t.join();
    }

    void run()
    {
        running = true;
        asio::io_context io;

        // Accept connections on port 8080
        tcp::acceptor acceptor(io, tcp::endpoint(tcp::v4(), 5000));
        std::cout << "Server is listening on port 5000...\n";

        while (running)
        {
            auto socket = std::make_shared<tcp::socket>(io);
            acceptor.accept(*socket);

            std::cout << "Client connected.\n";
            
            std::scoped_lock list(listen_lock);
            listeners.push_back([this, socket]()
            {
                char data[1024];
                std::size_t length = socket->read_some(asio::buffer(data));
                std::cout << "Received: " << length << " bytes\n";

                std::scoped_lock req(req_lock);
                requests.push_back(std::make_shared<Request>(
                    reinterpret_cast<const uint8_t*>(&data[0]),
                    length,
                    socket, 
                    this
                ));
            });
        }
    }

    template<typename Req, typename Res>
    void register_endpoint(const std::string& endpoint, std::function<void(typename Res::NativeTableType*, const typename Req::NativeTableType*)> func)
    {
        endpoints[endpoint] = [func](flatbuffers::FlatBufferBuilder& builder, const uint8_t* data)
        {
            typename Res::NativeTableType res;
            auto* req = flatbuffers::GetRoot<Req>(data);

            func(
                &res,
                req->UnPack()
            );

            flatbuffers::FlatBufferBuilder payload_builder;
            auto offset = Res::Pack(payload_builder, &res);
            payload_builder.Finish(offset);
            
            auto payload_vec = builder.CreateVector(
                payload_builder.GetBufferPointer(),
                payload_builder.GetSize()
            );

            auto response = App::CreateResponse(builder, 0, payload_vec);
            builder.Finish(response);
        };

        latency_metrics[endpoint] = &latency_family->Add({{ "endpoint", endpoint }});
    }

    std::string process_request(std::shared_ptr<tcp::socket> socket, const uint8_t* data) const
    {
        auto request = App::GetRequest(data);
        auto payload_vec = request->payload();
        std::cout << "Got endpoint " << request->endpoint()->str() << "\n";

        // If the endpoint doesn't exist, we should create a 
        auto builder = std::make_shared<flatbuffers::FlatBufferBuilder>();

        const auto& endpoint = request->endpoint()->str();
        if (!endpoints.count(endpoint))
        {
            auto response = App::CreateResponse(*builder, 1);
            builder->Finish(response);
        }
        else
            endpoints.at(endpoint)(*builder, payload_vec->Data());

        std::cout << "Writing response\n";
        asio::write(*socket, asio::buffer(builder->GetBufferPointer(), builder->GetSize()));

        return request->endpoint()->str();
    }
};

Request::Request(const uint8_t* data, size_t length, const std::shared_ptr<tcp::socket>& s, ServerInstance* server_instance) :
    socket(s),
    request_time(std::chrono::high_resolution_clock::now())
{
    buffer.resize(length);
    std::copy(data, data + length, buffer.begin());
    process = [server_instance](std::shared_ptr<Request> req)
    {
        const auto endpoint = server_instance->process_request(req->socket, &req->buffer[0]);
        
        if (server_instance->latency_metrics.count(endpoint))
        {
            const auto now = std::chrono::high_resolution_clock::now();
            std::chrono::duration<double> latency = now - req->request_time;
            server_instance->latency_metrics[endpoint]->Set(latency.count());
        }
    };
}

template<typename Res, typename Req>
std::pair<uint8_t, std::shared_ptr<typename Res::NativeTableType>>
get_response(const std::string& hostname, const std::string& endpoint, const typename Req::NativeTableType& payload)
{
    // Build Request
    flatbuffers::FlatBufferBuilder payload_builder, builder;
    auto offset = Req::Pack(payload_builder, &payload);
    payload_builder.Finish(offset);

    auto e = builder.CreateString(endpoint);
    auto payload_vec = builder.CreateVector(
        payload_builder.GetBufferPointer(),
        payload_builder.GetSize()
    );

    auto request = App::CreateRequest(builder, e, payload_vec);
    builder.Finish(request);

    asio::io_context io;

    tcp::resolver resolver(io);
    auto endpoints = resolver.resolve(hostname, "5000");

    tcp::socket socket(io);
    asio::connect(socket, endpoints);

    asio::write(socket, asio::buffer(
        builder.GetBufferPointer(),
        builder.GetSize()
    ));
    
    char response[1024];
    // Read the response from the server
    std::size_t length = socket.read_some(asio::buffer(response));
    
    auto* res = flatbuffers::GetRoot<App::Response>(response);

    const Res* res_payload = nullptr;
    if (!res->status())
        res_payload = flatbuffers::GetRoot<Res>(res->payload()->data());
    
    return { res->status(), std::shared_ptr<typename Res::NativeTableType>((res_payload ? res_payload->UnPack() : nullptr)) };
}   

}