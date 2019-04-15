#include <catch.hpp>
#include <zmq.hpp>
#ifdef ZMQ_CPP11
#include <future>
#endif

#if (__cplusplus >= 201703L)
static_assert(std::is_nothrow_swappable<zmq::socket_t>::value,
              "socket_t should be nothrow swappable");
#endif

TEST_CASE("socket default ctor", "[socket]")
{
    zmq::socket_t socket;
}

TEST_CASE("socket create destroy", "[socket]")
{
    zmq::context_t context;
    zmq::socket_t socket(context, ZMQ_ROUTER);
}

#ifdef ZMQ_CPP11
TEST_CASE("socket create assign", "[socket]")
{
    zmq::context_t context;
    zmq::socket_t socket(context, ZMQ_ROUTER);
    CHECK(static_cast<void*>(socket));
    socket = {};
    CHECK(!static_cast<void*>(socket));
}

TEST_CASE("socket create by enum and destroy", "[socket]")
{
    zmq::context_t context;
    zmq::socket_t socket(context, zmq::socket_type::router);
}

TEST_CASE("socket swap", "[socket]")
{
    zmq::context_t context;
    zmq::socket_t socket1(context, zmq::socket_type::router);
    zmq::socket_t socket2(context, zmq::socket_type::dealer);
    using std::swap;
    swap(socket1, socket2);
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

#ifdef ZMQ_CPP11
TEST_CASE("socket proxy", "[socket]")
{
    zmq::context_t context;
    zmq::socket_t front(context, ZMQ_ROUTER);
    zmq::socket_t back(context, ZMQ_ROUTER);
    zmq::socket_t capture(context, ZMQ_DEALER);
    front.bind("inproc://test1");
    back.bind("inproc://test2");
    capture.bind("inproc://test3");
    auto f = std::async(std::launch::async, [&]() {
        auto s1 = std::move(front);
        auto s2 = std::move(back);
        auto s3 = std::move(capture);
        try
        {
            zmq::proxy(s1, s2, &s3);
        }
        catch (const zmq::error_t& e)
        {
            return e.num() == ETERM;
        }
        return false;
    });
    context.close();
    CHECK(f.get());
}

TEST_CASE("socket proxy steerable", "[socket]")
{
    zmq::context_t context;
    zmq::socket_t front(context, ZMQ_ROUTER);
    zmq::socket_t back(context, ZMQ_ROUTER);
    zmq::socket_t control(context, ZMQ_SUB);
    front.bind("inproc://test1");
    back.bind("inproc://test2");
    control.connect("inproc://test3");
    auto f = std::async(std::launch::async, [&]() {
        auto s1 = std::move(front);
        auto s2 = std::move(back);
        auto s3 = std::move(control);
        try
        {
            zmq::proxy_steerable(s1, s2, ZMQ_NULLPTR, &s3);
        }
        catch (const zmq::error_t& e)
        {
            return e.num() == ETERM;
        }
        return false;
    });
    context.close();
    CHECK(f.get());
}
#endif
