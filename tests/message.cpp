#define CATCH_CONFIG_MAIN
#include <catch.hpp>
#include <zmq.hpp>

#if defined(ZMQ_CPP11)
static_assert(!std::is_copy_constructible<zmq::message_t>::value,
              "message_t should not be copy-constructible");
static_assert(!std::is_copy_assignable<zmq::message_t>::value,
              "message_t should not be copy-assignable");
#endif

TEST_CASE("message default constructed", "[message]")
{
    const zmq::message_t message;
    CHECK(0u == message.size());
}

namespace {
const char *const data = "Hi";
}

TEST_CASE("message constructor with iterators", "[message]")
{
    const std::string hi(data);
    const zmq::message_t hi_msg(hi.begin(), hi.end());
    CHECK(2u == hi_msg.size());
    CHECK(0 == memcmp(data, hi_msg.data(), 2));
}

TEST_CASE("message constructor with buffer and size", "[message]")
{
    const std::string hi(data);
    const zmq::message_t hi_msg(hi.data(), hi.size());
    CHECK(2u == hi_msg.size());
    CHECK(0 == memcmp(data, hi_msg.data(), 2));
}

TEST_CASE("message constructor with char array", "[message]")
{
    const zmq::message_t hi_msg(data, strlen(data));
    CHECK(2u == hi_msg.size());
    CHECK(0 == memcmp(data, hi_msg.data(), 2));
}

#if defined(ZMQ_BUILD_DRAFT_API) && defined(ZMQ_CPP11)
TEST_CASE("message constructor with container", "[message]")
{
    const std::string hi(data);
    zmq::message_t hi_msg(hi);
    CHECK(2u == hi_msg.size());
    CHECK(0 == memcmp(data, hi_msg.data(), 2));
}
#endif

#ifdef ZMQ_HAS_RVALUE_REFS
TEST_CASE("message move constructor", "[message]")
{
    zmq::message_t hi_msg(zmq::message_t(data, strlen(data)));
}

TEST_CASE("message assign move empty before", "[message]")
{
    zmq::message_t hi_msg;
    hi_msg = zmq::message_t(data, strlen(data));
    CHECK(2u == hi_msg.size());
    CHECK(0 == memcmp(data, hi_msg.data(), 2));
}

TEST_CASE("message assign move empty after", "[message]")
{
    zmq::message_t hi_msg(data, strlen(data));
    hi_msg = zmq::message_t();
    CHECK(0u == hi_msg.size());
}

TEST_CASE("message assign move empty before and after", "[message]")
{
    zmq::message_t hi_msg;
    hi_msg = zmq::message_t();
    CHECK(0u == hi_msg.size());
}
#endif

TEST_CASE("message equality self", "[message]")
{
    const zmq::message_t hi_msg(data, strlen(data));
    CHECK(hi_msg == hi_msg);
}

TEST_CASE("message equality equal", "[message]")
{
    const zmq::message_t hi_msg_a(data, strlen(data));
    const zmq::message_t hi_msg_b(data, strlen(data));
    CHECK(hi_msg_a == hi_msg_b);
}

TEST_CASE("message equality equal empty", "[message]")
{
    const zmq::message_t msg_a;
    const zmq::message_t msg_b;
    CHECK(msg_a == msg_b);
}

TEST_CASE("message equality non equal", "[message]")
{
    const zmq::message_t msg_a("Hi", 2);
    const zmq::message_t msg_b("Hello", 5);
    CHECK(msg_a != msg_b);
}

TEST_CASE("message equality non equal rhs empty", "[message]")
{
    const zmq::message_t msg_a("Hi", 2);
    const zmq::message_t msg_b;
    CHECK(msg_a != msg_b);
}

TEST_CASE("message equality non equal lhs empty", "[message]")
{
    const zmq::message_t msg_a;
    const zmq::message_t msg_b("Hi", 2);
    CHECK(msg_a != msg_b);
}

#if defined(ZMQ_BUILD_DRAFT_API) && ZMQ_VERSION >= ZMQ_MAKE_VERSION(4, 2, 0)
TEST_CASE("message routing id persists", "[message]")
{
    zmq::message_t msg;
    msg.set_routing_id(123);
    CHECK(123u == msg.routing_id());
}

TEST_CASE("message group persists", "[message]")
{
    zmq::message_t msg;
    msg.set_group("mygroup");
    CHECK(std::string(msg.group()) == "mygroup");
}
#endif
