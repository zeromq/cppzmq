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
    CHECK(static_cast<bool>(socket));
    CHECK(socket.handle() != nullptr);
    socket = {};
    CHECK(!static_cast<bool>(socket));
    CHECK(socket.handle() == nullptr);
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

TEST_CASE("socket flags", "[socket]")
{
    CHECK((zmq::recv_flags::dontwait | zmq::recv_flags::none)
          == static_cast<zmq::recv_flags>(ZMQ_DONTWAIT | 0));
    CHECK((zmq::recv_flags::dontwait & zmq::recv_flags::none)
          == static_cast<zmq::recv_flags>(ZMQ_DONTWAIT & 0));
    CHECK((zmq::recv_flags::dontwait ^ zmq::recv_flags::none)
          == static_cast<zmq::recv_flags>(ZMQ_DONTWAIT ^ 0));
    CHECK(~zmq::recv_flags::dontwait == static_cast<zmq::recv_flags>(~ZMQ_DONTWAIT));

    CHECK((zmq::send_flags::dontwait | zmq::send_flags::sndmore)
          == static_cast<zmq::send_flags>(ZMQ_DONTWAIT | ZMQ_SNDMORE));
    CHECK((zmq::send_flags::dontwait & zmq::send_flags::sndmore)
          == static_cast<zmq::send_flags>(ZMQ_DONTWAIT & ZMQ_SNDMORE));
    CHECK((zmq::send_flags::dontwait ^ zmq::send_flags::sndmore)
          == static_cast<zmq::send_flags>(ZMQ_DONTWAIT ^ ZMQ_SNDMORE));
    CHECK(~zmq::send_flags::dontwait == static_cast<zmq::send_flags>(~ZMQ_DONTWAIT));
}

TEST_CASE("socket readme example", "[socket]")
{
    zmq::context_t ctx;
    zmq::socket_t sock(ctx, zmq::socket_type::push);
    sock.bind("inproc://test");
    const std::string m = "Hello, world";
    sock.send(zmq::buffer(m), zmq::send_flags::dontwait);
}
#endif

TEST_CASE("socket sends and receives const buffer", "[socket]")
{
    zmq::context_t context;
    zmq::socket_t sender(context, ZMQ_PAIR);
    zmq::socket_t receiver(context, ZMQ_PAIR);
    receiver.bind("inproc://test");
    sender.connect("inproc://test");
    const char* str = "Hi";

    #ifdef ZMQ_CPP11
    CHECK(2 == *sender.send(zmq::buffer(str, 2)));
    char buf[2];
    const auto res = receiver.recv(zmq::buffer(buf));
    CHECK(res);
    CHECK(!res->truncated());
    CHECK(2 == res->size);
    #else
    CHECK(2 == sender.send(str, 2));
    char buf[2];
    CHECK(2 == receiver.recv(buf, 2));
    #endif
    CHECK(0 == memcmp(buf, str, 2));
}

#ifdef ZMQ_CPP11

TEST_CASE("socket send none sndmore", "[socket]")
{
    zmq::context_t context;
    zmq::socket_t s(context, zmq::socket_type::router);
    s.bind("inproc://test");

    std::vector<char> buf(4);
    auto res = s.send(zmq::buffer(buf), zmq::send_flags::sndmore);
    CHECK(res);
    CHECK(*res == buf.size());
    res = s.send(zmq::buffer(buf));
    CHECK(res);
    CHECK(*res == buf.size());
}

TEST_CASE("socket send dontwait", "[socket]")
{
    zmq::context_t context;
    zmq::socket_t s(context, zmq::socket_type::push);
    s.bind("inproc://test");

    std::vector<char> buf(4);
    auto res = s.send(zmq::buffer(buf), zmq::send_flags::dontwait);
    CHECK(!res);
    res = s.send(zmq::buffer(buf),
                  zmq::send_flags::dontwait | zmq::send_flags::sndmore);
    CHECK(!res);

    zmq::message_t msg;
    auto resm = s.send(msg, zmq::send_flags::dontwait);
    CHECK(!resm);
    CHECK(msg.size() == 0);
}

TEST_CASE("socket send exception", "[socket]")
{
    zmq::context_t context;
    zmq::socket_t s(context, zmq::socket_type::pull);
    s.bind("inproc://test");

    std::vector<char> buf(4);
    CHECK_THROWS_AS(s.send(zmq::buffer(buf)), const zmq::error_t &);
}

TEST_CASE("socket recv none", "[socket]")
{
    zmq::context_t context;
    zmq::socket_t s(context, zmq::socket_type::pair);
    zmq::socket_t s2(context, zmq::socket_type::pair);
    s2.bind("inproc://test");
    s.connect("inproc://test");

    std::vector<char> sbuf(4);
    const auto res_send = s2.send(zmq::buffer(sbuf));
    CHECK(res_send);
    CHECK(res_send.has_value());

    std::vector<char> buf(2);
    const auto res = s.recv(zmq::buffer(buf));
    CHECK(res.has_value());
    CHECK(res->truncated());
    CHECK(res->untruncated_size == sbuf.size());
    CHECK(res->size == buf.size());

    const auto res_send2 = s2.send(zmq::buffer(sbuf));
    CHECK(res_send2.has_value());
    std::vector<char> buf2(10);
    const auto res2 = s.recv(zmq::buffer(buf2));
    CHECK(res2.has_value());
    CHECK(!res2->truncated());
    CHECK(res2->untruncated_size == sbuf.size());
    CHECK(res2->size == sbuf.size());
}

TEST_CASE("socket send recv message_t", "[socket]")
{
    zmq::context_t context;
    zmq::socket_t s(context, zmq::socket_type::pair);
    zmq::socket_t s2(context, zmq::socket_type::pair);
    s2.bind("inproc://test");
    s.connect("inproc://test");

    zmq::message_t smsg(10);
    const auto res_send = s2.send(smsg, zmq::send_flags::none);
    CHECK(res_send);
    CHECK(*res_send == 10);
    CHECK(smsg.size() == 0);

    zmq::message_t rmsg;
    const auto res = s.recv(rmsg);
    CHECK(res);
    CHECK(*res == 10);
    CHECK(res.value() == 10);
    CHECK(rmsg.size() == *res);
}

TEST_CASE("socket send recv message_t by pointer", "[socket]")
{
    zmq::context_t context;
    zmq::socket_t s(context, zmq::socket_type::pair);
    zmq::socket_t s2(context, zmq::socket_type::pair);
    s2.bind("inproc://test");
    s.connect("inproc://test");

    zmq::message_t smsg(size_t{10});
    const auto res_send = s2.send(smsg, zmq::send_flags::none);
    CHECK(res_send);
    CHECK(*res_send == 10);
    CHECK(smsg.size() == 0);

    zmq::message_t rmsg;
    const bool res = s.recv(&rmsg);
    CHECK(res);
}

TEST_CASE("socket recv dontwait", "[socket]")
{
    zmq::context_t context;
    zmq::socket_t s(context, zmq::socket_type::pull);
    s.bind("inproc://test");

    std::vector<char> buf(4);
    constexpr auto flags = zmq::recv_flags::none | zmq::recv_flags::dontwait;
    auto res = s.recv(zmq::buffer(buf), flags);
    CHECK(!res);

    zmq::message_t msg;
    auto resm = s.recv(msg, flags);
    CHECK(!resm);
    CHECK_THROWS_AS(resm.value(), const std::exception &);
    CHECK(msg.size() == 0);
}

TEST_CASE("socket recv exception", "[socket]")
{
    zmq::context_t context;
    zmq::socket_t s(context, zmq::socket_type::push);
    s.bind("inproc://test");

    std::vector<char> buf(4);
    CHECK_THROWS_AS(s.recv(zmq::buffer(buf)), const zmq::error_t &);
}

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
            zmq::proxy(s1, s2, zmq::socket_ref(s3));
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
            zmq::proxy_steerable(s1, s2, zmq::socket_ref(), s3);
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
