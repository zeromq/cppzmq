#include <gtest/gtest.h>
#include <zmq.hpp>

TEST(socket, create_destroy)
{
    zmq::context_t context;
    zmq::socket_t socket(context, ZMQ_ROUTER);
}

