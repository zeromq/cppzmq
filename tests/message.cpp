#define CATCH_CONFIG_MAIN
#include <catch.hpp>
#include <zmq.hpp>

#if defined(ZMQ_CPP11)
static_assert(!std::is_copy_constructible<zmq::message_t>::value,
    "message_t should not be copy-constructible");
static_assert(!std::is_copy_assignable<zmq::message_t>::value,
    "message_t should not be copy-assignable");
#endif
#if (__cplusplus >= 201703L)
static_assert(std::is_nothrow_swappable<zmq::message_t>::value,
              "message_t should be nothrow swappable");
#endif

TEST_CASE("message default constructed", "[message]")
{
    const zmq::message_t message;
    CHECK(0u == message.size());
    CHECK(message.empty());
}

#ifdef ZMQ_CPP11
TEST_CASE("message swap", "[message]")
{
    const std::string data = "foo";
    zmq::message_t message1;
    zmq::message_t message2(data.data(), data.size());
    using std::swap;
    swap(message1, message2);
    CHECK(message1.size() == data.size());
    CHECK(message2.size() == 0);
    swap(message1, message2);
    CHECK(message1.size() == 0);
    CHECK(message2.size() == data.size());
}
#endif

namespace
{
const char *const data = "Hi";
}

TEST_CASE("message constructor with iterators", "[message]")
{
    const std::string hi(data);
    const zmq::message_t hi_msg(hi.begin(), hi.end());
    CHECK(2u == hi_msg.size());
    CHECK(0 == memcmp(data, hi_msg.data(), 2));
}

TEST_CASE("message constructor with size", "[message]")
{
    const zmq::message_t msg(5);
    CHECK(msg.size() == 5);
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

#if defined(ZMQ_CPP11) && !defined(ZMQ_CPP11_PARTIAL)
TEST_CASE("message constructor with container - deprecated", "[message]")
{
    zmq::message_t hi_msg("Hi"); // deprecated
    REQUIRE(3u == hi_msg.size());
    CHECK(0 == memcmp(data, hi_msg.data(), 3));
}

TEST_CASE("message constructor with container of trivial data", "[message]")
{
    int buf[3] = {1, 2, 3};
    zmq::message_t msg(buf);
    REQUIRE(sizeof(buf) == msg.size());
    CHECK(0 == memcmp(buf, msg.data(), msg.size()));
}

TEST_CASE("message constructor with strings", "[message]")
{
    SECTION("string")
    {
        const std::string hi(data);
        zmq::message_t hi_msg(hi);
        CHECK(2u == hi_msg.size());
        CHECK(0 == memcmp(data, hi_msg.data(), 2));
    }
#if CPPZMQ_HAS_STRING_VIEW
    SECTION("string_view")
    {
        const std::string_view hi(data);
        zmq::message_t hi_msg(hi);
        CHECK(2u == hi_msg.size());
        CHECK(0 == memcmp(data, hi_msg.data(), 2));
    }
#endif
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
    CHECK(!hi_msg.empty());
    hi_msg = zmq::message_t();
    CHECK(0u == hi_msg.size());
    CHECK(hi_msg.empty());
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

TEST_CASE("message to string", "[message]")
{
    const zmq::message_t a;
    const zmq::message_t b("Foo", 3);
    CHECK(a.to_string() == "");
    CHECK(b.to_string() == "Foo");
#if CPPZMQ_HAS_STRING_VIEW
    CHECK(a.to_string_view() == "");
    CHECK(b.to_string_view() == "Foo");
#endif

#if defined(ZMQ_CPP11) && !defined(ZMQ_CPP11_PARTIAL)
    const zmq::message_t depr("Foo"); // deprecated
    CHECK(depr.to_string() != "Foo");
    CHECK(depr.to_string() == std::string("Foo", 4));
#endif
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

#if ZMQ_VERSION >= ZMQ_MAKE_VERSION(3, 2, 0)
TEST_CASE("message is not shared", "[message]")
{
    zmq::message_t msg;
    CHECK(msg.get(ZMQ_SHARED) == 0);
}

TEST_CASE("message is shared", "[message]")
{
    size_t msg_sz = 1024; // large enough to be a type_lmsg
    zmq::message_t msg1(msg_sz);
    zmq::message_t msg2;
    msg2.copy(msg1);
    CHECK(msg1.get(ZMQ_SHARED) == 1);
    CHECK(msg2.get(ZMQ_SHARED) == 1);
    CHECK(msg1.size() == msg_sz);
    CHECK(msg2.size() == msg_sz);
}

TEST_CASE("message move is not shared", "[message]")
{
    size_t msg_sz = 1024; // large enough to be a type_lmsg
    zmq::message_t msg1(msg_sz);
    zmq::message_t msg2;
    msg2.move(msg1);
    CHECK(msg1.get(ZMQ_SHARED) == 0);
    CHECK(msg2.get(ZMQ_SHARED) == 0);
    CHECK(msg2.size() == msg_sz);
    CHECK(msg1.size() == 0);
}
#endif
