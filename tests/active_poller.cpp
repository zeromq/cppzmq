#include <zmq_addon.hpp>

#include "testutil.hpp"

#if defined(ZMQ_CPP11) && defined(ZMQ_BUILD_DRAFT_API)

#include <array>
#include <memory>

TEST_CASE("create destroy", "[active_poller]")
{
    zmq::active_poller_t active_poller;
    CHECK(active_poller.empty());
}

static_assert(!std::is_copy_constructible<zmq::active_poller_t>::value,
              "active_poller_t should not be copy-constructible");
static_assert(!std::is_copy_assignable<zmq::active_poller_t>::value,
              "active_poller_t should not be copy-assignable");

static const zmq::active_poller_t::handler_type no_op_handler =
  [](zmq::event_flags) {};

TEST_CASE("move construct empty", "[active_poller]")
{
    zmq::active_poller_t a;
    CHECK(a.empty());
    zmq::active_poller_t b = std::move(a);
    CHECK(b.empty());
    CHECK(0u == a.size());
    CHECK(0u == b.size());
}

TEST_CASE("move assign empty", "[active_poller]")
{
    zmq::active_poller_t a;
    CHECK(a.empty());
    zmq::active_poller_t b;
    CHECK(b.empty());
    b = std::move(a);
    CHECK(0u == a.size());
    CHECK(0u == b.size());
    CHECK(a.empty());
    CHECK(b.empty());
}

TEST_CASE("move construct non empty", "[active_poller]")
{
    zmq::context_t context;
    zmq::socket_t socket{context, zmq::socket_type::router};

    zmq::active_poller_t a;
    a.add(socket, zmq::event_flags::pollin, [](zmq::event_flags) {});
    CHECK_FALSE(a.empty());
    CHECK(1u == a.size());
    zmq::active_poller_t b = std::move(a);
    CHECK(a.empty());
    CHECK(0u == a.size());
    CHECK_FALSE(b.empty());
    CHECK(1u == b.size());
}

TEST_CASE("move assign non empty", "[active_poller]")
{
    zmq::context_t context;
    zmq::socket_t socket{context, zmq::socket_type::router};

    zmq::active_poller_t a;
    a.add(socket, zmq::event_flags::pollin, no_op_handler);
    CHECK_FALSE(a.empty());
    CHECK(1u == a.size());
    zmq::active_poller_t b;
    b = std::move(a);
    CHECK(a.empty());
    CHECK(0u == a.size());
    CHECK_FALSE(b.empty());
    CHECK(1u == b.size());
}

TEST_CASE("add handler", "[active_poller]")
{
    zmq::context_t context;
    zmq::socket_t socket{context, zmq::socket_type::router};
    zmq::active_poller_t active_poller;
    CHECK_NOTHROW(
      active_poller.add(socket, zmq::event_flags::pollin, no_op_handler));
}

TEST_CASE("add null handler fails", "[active_poller]")
{
    zmq::context_t context;
    zmq::socket_t socket{context, zmq::socket_type::router};
    zmq::active_poller_t active_poller;
    zmq::active_poller_t::handler_type handler;
    CHECK_THROWS_AS(active_poller.add(socket, zmq::event_flags::pollin, handler),
                    const std::invalid_argument &);
}

#if ZMQ_VERSION >= ZMQ_MAKE_VERSION(4, 3, 0)
// this behaviour was added by https://github.com/zeromq/libzmq/pull/3100
TEST_CASE("add handler invalid events type", "[active_poller]")
{
    zmq::context_t context;
    zmq::socket_t socket{context, zmq::socket_type::router};
    zmq::active_poller_t active_poller;
    short invalid_events_type = 2 << 10;
    CHECK_THROWS_AS(
      active_poller.add(socket, static_cast<zmq::event_flags>(invalid_events_type),
                        no_op_handler),
      const zmq::error_t &);
    CHECK(active_poller.empty());
    CHECK(0u == active_poller.size());
}
#endif

TEST_CASE("add handler twice throws", "[active_poller]")
{
    common_server_client_setup s;

    CHECK(s.client.send(zmq::message_t{}, zmq::send_flags::none));

    zmq::active_poller_t active_poller;
    bool message_received = false;
    active_poller.add(
      s.server, zmq::event_flags::pollin,
      [&message_received](zmq::event_flags) { message_received = true; });
    CHECK_THROWS_ZMQ_ERROR(
      EINVAL, active_poller.add(s.server, zmq::event_flags::pollin, no_op_handler));
    CHECK(1 == active_poller.wait(std::chrono::milliseconds{-1}));
    CHECK(message_received); // handler unmodified
}

TEST_CASE("wait with no handlers throws", "[active_poller]")
{
    zmq::active_poller_t active_poller;
    CHECK_THROWS_ZMQ_ERROR(EFAULT,
                           active_poller.wait(std::chrono::milliseconds{10}));
}

TEST_CASE("remove unregistered throws", "[active_poller]")
{
    zmq::context_t context;
    zmq::socket_t socket{context, zmq::socket_type::router};
    zmq::active_poller_t active_poller;
    CHECK_THROWS_ZMQ_ERROR(EINVAL, active_poller.remove(socket));
}

TEST_CASE("remove registered empty", "[active_poller]")
{
    zmq::context_t context;
    zmq::socket_t socket{context, zmq::socket_type::router};
    zmq::active_poller_t active_poller;
    active_poller.add(socket, zmq::event_flags::pollin, no_op_handler);
    CHECK_NOTHROW(active_poller.remove(socket));
}

TEST_CASE("remove registered non empty", "[active_poller]")
{
    zmq::context_t context;
    zmq::socket_t socket{context, zmq::socket_type::router};
    zmq::active_poller_t active_poller;
    active_poller.add(socket, zmq::event_flags::pollin, no_op_handler);
    CHECK_NOTHROW(active_poller.remove(socket));
}

namespace
{
struct server_client_setup : common_server_client_setup
{
    zmq::active_poller_t::handler_type handler = [&](zmq::event_flags e) {
        events = e;
    };

    zmq::event_flags events = zmq::event_flags::none;
};

const std::string hi_str = "Hi";

}

TEST_CASE("poll basic", "[active_poller]")
{
    server_client_setup s;

    CHECK_NOTHROW(s.client.send(zmq::message_t{hi_str}, zmq::send_flags::none));

    zmq::active_poller_t active_poller;
    bool message_received = false;
    zmq::active_poller_t::handler_type handler =
      [&message_received](zmq::event_flags events) {
          CHECK(zmq::event_flags::none != (events & zmq::event_flags::pollin));
          message_received = true;
      };
    CHECK_NOTHROW(active_poller.add(s.server, zmq::event_flags::pollin, handler));
    CHECK(1 == active_poller.wait(std::chrono::milliseconds{-1}));
    CHECK(message_received);
}

/// \todo this contains multiple test cases that should be split up
TEST_CASE("client server", "[active_poller]")
{
    const std::string send_msg = hi_str;

    // Setup server and client
    server_client_setup s;

    // Setup active_poller
    zmq::active_poller_t active_poller;
    zmq::event_flags events;
    zmq::active_poller_t::handler_type handler = [&](zmq::event_flags e) {
        if (zmq::event_flags::none != (e & zmq::event_flags::pollin)) {
            zmq::message_t zmq_msg;
            CHECK_NOTHROW(s.server.recv(zmq_msg)); // get message
            std::string recv_msg(zmq_msg.data<char>(), zmq_msg.size());
            CHECK(send_msg == recv_msg);
        } else if (zmq::event_flags::none != (e & ~zmq::event_flags::pollout)) {
            INFO("Unexpected event type " << static_cast<short>(events));
            REQUIRE(false);
        }
        events = e;
    };

    CHECK_NOTHROW(active_poller.add(s.server, zmq::event_flags::pollin, handler));

    // client sends message
    CHECK_NOTHROW(s.client.send(zmq::message_t{send_msg}, zmq::send_flags::none));

    CHECK(1 == active_poller.wait(std::chrono::milliseconds{-1}));
    CHECK(events == zmq::event_flags::pollin);

    // Re-add server socket with pollout flag
    CHECK_NOTHROW(active_poller.remove(s.server));
    CHECK_NOTHROW(active_poller.add(
      s.server, zmq::event_flags::pollin | zmq::event_flags::pollout, handler));
    CHECK(1 == active_poller.wait(std::chrono::milliseconds{-1}));
    CHECK(events == zmq::event_flags::pollout);
}

TEST_CASE("add invalid socket throws", "[active_poller]")
{
    zmq::context_t context;
    zmq::active_poller_t active_poller;
    zmq::socket_t a{context, zmq::socket_type::router};
    zmq::socket_t b{std::move(a)};
    CHECK_THROWS_AS(active_poller.add(a, zmq::event_flags::pollin, no_op_handler),
                    const zmq::error_t &);
}

TEST_CASE("remove invalid socket throws", "[active_poller]")
{
    zmq::context_t context;
    zmq::socket_t socket{context, zmq::socket_type::router};
    zmq::active_poller_t active_poller;
    CHECK_NOTHROW(
      active_poller.add(socket, zmq::event_flags::pollin, no_op_handler));
    CHECK(1u == active_poller.size());
    std::vector<zmq::socket_t> sockets;
    sockets.emplace_back(std::move(socket));
    CHECK_THROWS_AS(active_poller.remove(socket), const zmq::error_t &);
    CHECK(1u == active_poller.size());
}

TEST_CASE("wait on added empty handler", "[active_poller]")
{
    server_client_setup s;
    CHECK_NOTHROW(s.client.send(zmq::message_t{hi_str}, zmq::send_flags::none));
    zmq::active_poller_t active_poller;
    CHECK_NOTHROW(
      active_poller.add(s.server, zmq::event_flags::pollin, no_op_handler));
    CHECK_NOTHROW(active_poller.wait(std::chrono::milliseconds{-1}));
}

TEST_CASE("modify empty throws", "[active_poller]")
{
    zmq::context_t context;
    zmq::socket_t socket{context, zmq::socket_type::push};
    zmq::active_poller_t active_poller;
    CHECK_THROWS_AS(active_poller.modify(socket, zmq::event_flags::pollin),
                    const zmq::error_t &);
}

TEST_CASE("modify invalid socket throws", "[active_poller]")
{
    zmq::context_t context;
    zmq::socket_t a{context, zmq::socket_type::push};
    zmq::socket_t b{std::move(a)};
    zmq::active_poller_t active_poller;
    CHECK_THROWS_AS(active_poller.modify(a, zmq::event_flags::pollin),
                    const zmq::error_t &);
}

TEST_CASE("modify not added throws", "[active_poller]")
{
    zmq::context_t context;
    zmq::socket_t a{context, zmq::socket_type::push};
    zmq::socket_t b{context, zmq::socket_type::push};
    zmq::active_poller_t active_poller;
    CHECK_NOTHROW(active_poller.add(a, zmq::event_flags::pollin, no_op_handler));
    CHECK_THROWS_AS(active_poller.modify(b, zmq::event_flags::pollin),
                    const zmq::error_t &);
}

TEST_CASE("modify simple", "[active_poller]")
{
    zmq::context_t context;
    zmq::socket_t a{context, zmq::socket_type::push};
    zmq::active_poller_t active_poller;
    CHECK_NOTHROW(active_poller.add(a, zmq::event_flags::pollin, no_op_handler));
    CHECK_NOTHROW(
      active_poller.modify(a, zmq::event_flags::pollin | zmq::event_flags::pollout));
}

TEST_CASE("poll client server", "[active_poller]")
{
    // Setup server and client
    server_client_setup s;

    // Setup active_poller
    zmq::active_poller_t active_poller;
    CHECK_NOTHROW(active_poller.add(s.server, zmq::event_flags::pollin, s.handler));

    // client sends message
    CHECK_NOTHROW(s.client.send(zmq::message_t{hi_str}, zmq::send_flags::none));

    // wait for message and verify events
    CHECK_NOTHROW(active_poller.wait(std::chrono::milliseconds{500}));
    CHECK(s.events == zmq::event_flags::pollin);

    // Modify server socket with pollout flag
    CHECK_NOTHROW(active_poller.modify(s.server, zmq::event_flags::pollin
                                                   | zmq::event_flags::pollout));
    CHECK(1 == active_poller.wait(std::chrono::milliseconds{500}));
    CHECK(s.events == (zmq::event_flags::pollin | zmq::event_flags::pollout));
}

TEST_CASE("wait one return", "[active_poller]")
{
    // Setup server and client
    server_client_setup s;

    int count = 0;

    // Setup active_poller
    zmq::active_poller_t active_poller;
    CHECK_NOTHROW(active_poller.add(s.server, zmq::event_flags::pollin,
                                    [&count](zmq::event_flags) { ++count; }));

    // client sends message
    CHECK_NOTHROW(s.client.send(zmq::message_t{hi_str}, zmq::send_flags::none));

    // wait for message and verify events
    CHECK(1 == active_poller.wait(std::chrono::milliseconds{500}));
    CHECK(1u == count);
}

TEST_CASE("wait on move constructed active_poller", "[active_poller]")
{
    server_client_setup s;
    CHECK_NOTHROW(s.client.send(zmq::message_t{hi_str}, zmq::send_flags::none));
    zmq::active_poller_t a;
    CHECK_NOTHROW(a.add(s.server, zmq::event_flags::pollin, no_op_handler));
    zmq::active_poller_t b{std::move(a)};
    CHECK(1u == b.size());
    CHECK(0u == a.size());
    CHECK_THROWS_ZMQ_ERROR(EFAULT, a.wait(std::chrono::milliseconds{10}));
    CHECK(b.wait(std::chrono::milliseconds{-1}));
}

TEST_CASE("wait on move assigned active_poller", "[active_poller]")
{
    server_client_setup s;
    CHECK_NOTHROW(s.client.send(zmq::message_t{hi_str}, zmq::send_flags::none));
    zmq::active_poller_t a;
    CHECK_NOTHROW(a.add(s.server, zmq::event_flags::pollin, no_op_handler));
    zmq::active_poller_t b;
    b = {std::move(a)};
    CHECK(1u == b.size());
    CHECK(0u == a.size());
    CHECK_THROWS_ZMQ_ERROR(EFAULT, a.wait(std::chrono::milliseconds{10}));
    CHECK(b.wait(std::chrono::milliseconds{-1}));
}

TEST_CASE("received on move constructed active_poller", "[active_poller]")
{
    // Setup server and client
    server_client_setup s;
    int count = 0;
    // Setup active_poller a
    zmq::active_poller_t a;
    CHECK_NOTHROW(a.add(s.server, zmq::event_flags::pollin,
                        [&count](zmq::event_flags) { ++count; }));
    // client sends message
    CHECK_NOTHROW(s.client.send(zmq::message_t{hi_str}, zmq::send_flags::none));
    // wait for message and verify it is received
    CHECK(1 == a.wait(std::chrono::milliseconds{500}));
    CHECK(1u == count);
    // Move construct active_poller b
    zmq::active_poller_t b{std::move(a)};
    // client sends message again
    CHECK_NOTHROW(s.client.send(zmq::message_t{hi_str}, zmq::send_flags::none));
    // wait for message and verify it is received
    CHECK(1 == b.wait(std::chrono::milliseconds{500}));
    CHECK(2u == count);
}


TEST_CASE("remove from handler", "[active_poller]")
{
    constexpr size_t ITER_NO = 10;

    // Setup servers and clients
    std::vector<server_client_setup> setup_list;
    for (size_t i = 0; i < ITER_NO; ++i)
        setup_list.emplace_back(server_client_setup{});

    // Setup active_poller
    zmq::active_poller_t active_poller;
    int count = 0;
    for (size_t i = 0; i < ITER_NO; ++i) {
        CHECK_NOTHROW(active_poller.add(
          setup_list[i].server, zmq::event_flags::pollin,
          [&, i](zmq::event_flags events) {
              CHECK(events == zmq::event_flags::pollin);
              active_poller.remove(setup_list[ITER_NO - i - 1].server);
              CHECK((ITER_NO - i - 1) == active_poller.size());
          }));
        ++count;
    }
    CHECK(ITER_NO == active_poller.size());
    // Clients send messages
    for (auto &s : setup_list) {
        CHECK_NOTHROW(s.client.send(zmq::message_t{hi_str}, zmq::send_flags::none));
    }

    // Wait for all servers to receive a message
    for (auto &s : setup_list) {
        zmq::pollitem_t items[] = {{s.server, 0, ZMQ_POLLIN, 0}};
        zmq::poll(&items[0], 1);
    }

    // Fire all handlers in one wait
    CHECK(ITER_NO == active_poller.wait(std::chrono::milliseconds{-1}));
    CHECK(ITER_NO == count);
}

#endif
