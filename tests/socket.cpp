#include <gtest/gtest.h>
#include <zmq.hpp>

TEST(socket, create_destroy)
{
    zmq::context_t context;
    zmq::socket_t socket(context, ZMQ_ROUTER);
}

#ifdef ZMQ_CPP11
TEST(socket, create_by_enum_destroy)
{
    zmq::context_t context;
    zmq::socket_t socket(context, zmq::socket_type::router);
}
#endif
