#include <catch.hpp>
#include <zmq_addon.hpp>
#ifdef ZMQ_CPP11

#include <forward_list>

TEST_CASE("send_multipart test", "[send_multipart]")
{
    zmq::context_t context(1);
    zmq::socket_t output(context, ZMQ_PAIR);
    zmq::socket_t input(context, ZMQ_PAIR);
    output.bind("inproc://multipart.test");
    input.connect("inproc://multipart.test");

    SECTION("send 0 messages") {
        std::vector<zmq::message_t> imsgs;
        auto iret = zmq::send_multipart(input, imsgs);
        REQUIRE(iret);
        CHECK(*iret == 0);
    }
    SECTION("send 1 message") {
        std::array<zmq::message_t, 1> imsgs = {zmq::message_t(3)};
        auto iret = zmq::send_multipart(input, imsgs);
        REQUIRE(iret);
        CHECK(*iret == 1);

        std::vector<zmq::message_t> omsgs;
        auto oret = zmq::recv_multipart(output, std::back_inserter(omsgs));
        REQUIRE(oret);
        CHECK(*oret == 1);
        REQUIRE(omsgs.size() == 1);
        CHECK(omsgs[0].size() == 3);
    }
    SECTION("send 2 messages") {
        std::array<zmq::message_t, 2> imsgs = {zmq::message_t(3), zmq::message_t(4)};
        auto iret = zmq::send_multipart(input, imsgs);
        REQUIRE(iret);
        CHECK(*iret == 2);

        std::vector<zmq::message_t> omsgs;
        auto oret = zmq::recv_multipart(output, std::back_inserter(omsgs));
        REQUIRE(oret);
        CHECK(*oret == 2);
        REQUIRE(omsgs.size() == 2);
        CHECK(omsgs[0].size() == 3);
        CHECK(omsgs[1].size() == 4);
    }
    SECTION("send 2 messages, const_buffer") {
        std::array<zmq::const_buffer, 2> imsgs = {zmq::str_buffer("foo"),
                                                  zmq::str_buffer("bar!")};
        auto iret = zmq::send_multipart(input, imsgs);
        REQUIRE(iret);
        CHECK(*iret == 2);

        std::vector<zmq::message_t> omsgs;
        auto oret = zmq::recv_multipart(output, std::back_inserter(omsgs));
        REQUIRE(oret);
        CHECK(*oret == 2);
        REQUIRE(omsgs.size() == 2);
        CHECK(omsgs[0].size() == 3);
        CHECK(omsgs[1].size() == 4);
    }
    SECTION("send 2 messages, mutable_buffer") {
        char buf[4] = {};
        std::array<zmq::mutable_buffer, 2> imsgs = {
            zmq::buffer(buf, 3), zmq::buffer(buf)};
        auto iret = zmq::send_multipart(input, imsgs);
        REQUIRE(iret);
        CHECK(*iret == 2);

        std::vector<zmq::message_t> omsgs;
        auto oret = zmq::recv_multipart(output, std::back_inserter(omsgs));
        REQUIRE(oret);
        CHECK(*oret == 2);
        REQUIRE(omsgs.size() == 2);
        CHECK(omsgs[0].size() == 3);
        CHECK(omsgs[1].size() == 4);
    }
    SECTION("send 2 messages, dontwait") {
        zmq::socket_t push(context, ZMQ_PUSH);
        push.bind("inproc://multipart.test.push");

        std::array<zmq::message_t, 2> imsgs = {zmq::message_t(3), zmq::message_t(4)};
        auto iret = zmq::send_multipart(push, imsgs, zmq::send_flags::dontwait);
        REQUIRE_FALSE(iret);
    }
    SECTION("send, misc. containers") {
        std::vector<zmq::message_t> msgs_vec;
        msgs_vec.emplace_back(3);
        msgs_vec.emplace_back(4);
        auto iret = zmq::send_multipart(input, msgs_vec);
        REQUIRE(iret);
        CHECK(*iret == 2);

        std::forward_list<zmq::message_t> msgs_list;
        msgs_list.emplace_front(4);
        msgs_list.emplace_front(3);
        iret = zmq::send_multipart(input, msgs_list);
        REQUIRE(iret);
        CHECK(*iret == 2);

        // init. list
        const auto msgs_il = {zmq::str_buffer("foo"), zmq::str_buffer("bar!")};
        iret = zmq::send_multipart(input, msgs_il);
        REQUIRE(iret);
        CHECK(*iret == 2);
        // rvalue
        iret = zmq::send_multipart(input,
                                   std::initializer_list<zmq::const_buffer>{
                                       zmq::str_buffer("foo"),
                                       zmq::str_buffer("bar!")});
        REQUIRE(iret);
        CHECK(*iret == 2);
    }
    SECTION("send with invalid socket") {
        std::vector<zmq::message_t> msgs(1);
        CHECK_THROWS_AS(zmq::send_multipart(zmq::socket_ref(), msgs),
                        const zmq::error_t &);
    }
}
#endif
