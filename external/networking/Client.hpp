#pragma once

#include <date/date.h>

#include <asio.hpp>
#include <iostream>
#include <unordered_map>

#include <flatbuffers/flatbuffers.h>

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