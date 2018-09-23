#include <catch.hpp>
#include <zmq.hpp>

TEST_CASE("socket create destroy", "[socket]")
{
    zmq::context_t context;
    zmq::socket_t socket(context, ZMQ_ROUTER);
}

#ifdef ZMQ_CPP11
TEST_CASE("socket create by enum and destroy", "[socket]")
{
    zmq::context_t context;
    zmq::socket_t socket(context, zmq::socket_type::router);
}
#endif

TEST_CASE("socket sends and receives const buffer", "[socket]")
{
    zmq::context_t context;
    zmq::socket_t sender(context, ZMQ_PAIR);
    zmq::socket_t receiver(context, ZMQ_PAIR);
    receiver.bind("inproc://test");
    sender.connect("inproc://test");
    CHECK(2 == sender.send("Hi", 2));
    char buf[2];
    CHECK(2 == receiver.recv(buf, 2));
    CHECK(0 == memcmp(buf, "Hi", 2));
}
