#pragma once

#include <catch.hpp>
#include <zmq.hpp>

#if defined(ZMQ_CPP11)

inline std::string bind_ip4_loopback(zmq::socket_t &socket)
{
    socket.bind("tcp://127.0.0.1:*");
    std::string endpoint(100, ' ');
    endpoint.resize(socket.get(zmq::sockopt::last_endpoint, zmq::buffer(endpoint)));
    return endpoint;
}

struct common_server_client_setup
{
    common_server_client_setup(bool initialize = true)
    {
        if (initialize)
            init();
    }

    void init()
    {
        endpoint = bind_ip4_loopback(server);
        REQUIRE_NOTHROW(client.connect(endpoint));
    }

    zmq::context_t context;
    zmq::socket_t server{context, zmq::socket_type::pair};
    zmq::socket_t client{context, zmq::socket_type::pair};
    std::string endpoint;
};
#endif
