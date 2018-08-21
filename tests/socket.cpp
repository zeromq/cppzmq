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

TEST(socket, send_receive_const_buffer)
{
    zmq::context_t context;
    zmq::socket_t sender(context, ZMQ_PAIR);
    zmq::socket_t receiver(context, ZMQ_PAIR);
    receiver.bind("inproc://test");
    sender.connect("inproc://test");
    ASSERT_EQ(2, sender.send("Hi", 2));
    char buf[2];
    ASSERT_EQ(2, receiver.recv(buf, 2));
    ASSERT_EQ(0, memcmp(buf, "Hi", 2));
}
