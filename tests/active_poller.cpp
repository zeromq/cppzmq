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
              "active_active_poller_t should not be copy-constructible");
static_assert(!std::is_copy_assignable<zmq::active_poller_t>::value,
              "active_active_poller_t should not be copy-assignable");

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
    a.add(socket, ZMQ_POLLIN, [](short) {});
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
    a.add(socket, ZMQ_POLLIN, [](short) {});
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
    zmq::active_poller_t::handler_t handler;
    CHECK_NOTHROW(active_poller.add(socket, ZMQ_POLLIN, handler));
}

#if ZMQ_VERSION >= ZMQ_MAKE_VERSION(4, 3, 0)
// this behaviour was added by https://github.com/zeromq/libzmq/pull/3100
TEST_CASE("add handler invalid events type", "[active_poller]")
{
    zmq::context_t context;
    zmq::socket_t socket{context, zmq::socket_type::router};
    zmq::active_poller_t active_poller;
    zmq::active_poller_t::handler_t handler;
    short invalid_events_type = 2 << 10;
    CHECK_THROWS_AS(active_poller.add(socket, invalid_events_type, handler), zmq::error_t);
    CHECK(active_poller.empty());
    CHECK(0u == active_poller.size());
}
#endif

TEST_CASE("add handler twice throws", "[active_poller]")
{
    zmq::context_t context;
    zmq::socket_t socket{context, zmq::socket_type::router};
    zmq::active_poller_t active_poller;
    zmq::active_poller_t::handler_t handler;
    active_poller.add(socket, ZMQ_POLLIN, handler);
    /// \todo the actual error code should be checked
    CHECK_THROWS_AS(active_poller.add(socket, ZMQ_POLLIN, handler), zmq::error_t);
}

TEST_CASE("wait with no handlers throws", "[active_poller]")
{
    zmq::active_poller_t active_poller;
    /// \todo the actual error code should be checked
    CHECK_THROWS_AS(active_poller.wait(std::chrono::milliseconds{10}), zmq::error_t);
}

TEST_CASE("remove unregistered throws", "[active_poller]")
{
    zmq::context_t context;
    zmq::socket_t socket{context, zmq::socket_type::router};
    zmq::active_poller_t active_poller;
    /// \todo the actual error code should be checked
    CHECK_THROWS_AS(active_poller.remove(socket), zmq::error_t);
}

TEST_CASE("remove registered empty", "[active_poller]")
{
    zmq::context_t context;
    zmq::socket_t socket{context, zmq::socket_type::router};
    zmq::active_poller_t active_poller;
    active_poller.add(socket, ZMQ_POLLIN, zmq::active_poller_t::handler_t{});
    CHECK_NOTHROW(active_poller.remove(socket));
}

TEST_CASE("remove registered non empty", "[active_poller]")
{
    zmq::context_t context;
    zmq::socket_t socket{context, zmq::socket_type::router};
    zmq::active_poller_t active_poller;
    active_poller.add(socket, ZMQ_POLLIN, [](short) {});
    CHECK_NOTHROW(active_poller.remove(socket));
}

namespace
{
struct server_client_setup : common_server_client_setup
{
    zmq::active_poller_t::handler_t handler = [&](short e) { events = e; };

    short events = 0;
};
}

TEST_CASE("poll basic", "[active_poller]")
{
    server_client_setup s;

    CHECK_NOTHROW(s.client.send(zmq::message_t{"Hi"}));

    zmq::active_poller_t active_poller;
    bool message_received = false;
    zmq::active_poller_t::handler_t handler = [&message_received](short events) {
        CHECK(0 != (events & ZMQ_POLLIN));
        message_received = true;
    };
    CHECK_NOTHROW(active_poller.add(s.server, ZMQ_POLLIN, handler));
    CHECK(1 == active_poller.wait(std::chrono::milliseconds{-1}));
    CHECK(message_received);
}

/// \todo this contains multiple test cases that should be split up
TEST_CASE("client server", "[active_poller]")
{
    const std::string send_msg = "Hi";

    // Setup server and client
    server_client_setup s;

    // Setup active_poller
    zmq::active_poller_t active_poller;
    short events;
    zmq::active_poller_t::handler_t handler = [&](short e) {
        if (0 != (e & ZMQ_POLLIN)) {
            zmq::message_t zmq_msg;
            CHECK_NOTHROW(s.server.recv(&zmq_msg)); // get message
            std::string recv_msg(zmq_msg.data<char>(), zmq_msg.size());
            CHECK(send_msg == recv_msg);
        } else if (0 != (e & ~ZMQ_POLLOUT)) {
            INFO("Unexpected event type " << events);
            REQUIRE(false);
        }
        events = e;
    };

    CHECK_NOTHROW(active_poller.add(s.server, ZMQ_POLLIN, handler));

    // client sends message
    CHECK_NOTHROW(s.client.send(zmq::message_t{send_msg}));

    CHECK(1 == active_poller.wait(std::chrono::milliseconds{-1}));
    CHECK(events == ZMQ_POLLIN);

    // Re-add server socket with pollout flag
    CHECK_NOTHROW(active_poller.remove(s.server));
    CHECK_NOTHROW(active_poller.add(s.server, ZMQ_POLLIN | ZMQ_POLLOUT, handler));
    CHECK(1 == active_poller.wait(std::chrono::milliseconds{-1}));
    CHECK(events == ZMQ_POLLOUT);
}

TEST_CASE("add invalid socket throws", "[active_poller]")
{
    zmq::context_t context;
    zmq::active_poller_t active_poller;
    zmq::socket_t a{context, zmq::socket_type::router};
    zmq::socket_t b{std::move(a)};
    CHECK_THROWS_AS(active_poller.add(a, ZMQ_POLLIN, zmq::active_poller_t::handler_t{}),
                 zmq::error_t);
}

TEST_CASE("remove invalid socket throws", "[active_poller]")
{
    zmq::context_t context;
    zmq::socket_t socket{context, zmq::socket_type::router};
    zmq::active_poller_t active_poller;
    CHECK_NOTHROW(
      active_poller.add(socket, ZMQ_POLLIN, zmq::active_poller_t::handler_t{}));
    CHECK(1u == active_poller.size());
    std::vector<zmq::socket_t> sockets;
    sockets.emplace_back(std::move(socket));
    CHECK_THROWS_AS(active_poller.remove(socket), zmq::error_t);
    CHECK(1u == active_poller.size());
}

TEST_CASE("wait on added empty handler", "[active_poller]")
{
    server_client_setup s;
    CHECK_NOTHROW(s.client.send(zmq::message_t{"Hi"}));
    zmq::active_poller_t active_poller;
    zmq::active_poller_t::handler_t handler;
    CHECK_NOTHROW(active_poller.add(s.server, ZMQ_POLLIN, handler));
    CHECK_NOTHROW(active_poller.wait(std::chrono::milliseconds{-1}));
}

TEST_CASE("modify empty throws", "[active_poller]")
{
    zmq::context_t context;
    zmq::socket_t socket{context, zmq::socket_type::push};
    zmq::active_poller_t active_poller;
    CHECK_THROWS_AS(active_poller.modify(socket, ZMQ_POLLIN), zmq::error_t);
}

TEST_CASE("modify invalid socket throws", "[active_poller]")
{
    zmq::context_t context;
    zmq::socket_t a{context, zmq::socket_type::push};
    zmq::socket_t b{std::move(a)};
    zmq::active_poller_t active_poller;
    CHECK_THROWS_AS(active_poller.modify(a, ZMQ_POLLIN), zmq::error_t);
}

TEST_CASE("modify not added throws", "[active_poller]")
{
    zmq::context_t context;
    zmq::socket_t a{context, zmq::socket_type::push};
    zmq::socket_t b{context, zmq::socket_type::push};
    zmq::active_poller_t active_poller;
    CHECK_NOTHROW(
      active_poller.add(a, ZMQ_POLLIN, zmq::active_poller_t::handler_t{}));
    CHECK_THROWS_AS(active_poller.modify(b, ZMQ_POLLIN), zmq::error_t);
}

TEST_CASE("modify simple", "[active_poller]")
{
    zmq::context_t context;
    zmq::socket_t a{context, zmq::socket_type::push};
    zmq::active_poller_t active_poller;
    CHECK_NOTHROW(
      active_poller.add(a, ZMQ_POLLIN, zmq::active_poller_t::handler_t{}));
    CHECK_NOTHROW(active_poller.modify(a, ZMQ_POLLIN | ZMQ_POLLOUT));
}

TEST_CASE("poll client server", "[active_poller]")
{
    // Setup server and client
    server_client_setup s;

    // Setup active_poller
    zmq::active_poller_t active_poller;
    CHECK_NOTHROW(active_poller.add(s.server, ZMQ_POLLIN, s.handler));

    // client sends message
    CHECK_NOTHROW(s.client.send(zmq::message_t{"Hi"}));

    // wait for message and verify events
    CHECK_NOTHROW(active_poller.wait(std::chrono::milliseconds{500}));
    CHECK(s.events == ZMQ_POLLIN);

    // Modify server socket with pollout flag
    CHECK_NOTHROW(active_poller.modify(s.server, ZMQ_POLLIN | ZMQ_POLLOUT));
    CHECK(1 == active_poller.wait(std::chrono::milliseconds{500}));
    CHECK(s.events == (ZMQ_POLLIN | ZMQ_POLLOUT));
}

TEST_CASE("wait one return", "[active_poller]")
{
    // Setup server and client
    server_client_setup s;

    int count = 0;

    // Setup active_poller
    zmq::active_poller_t active_poller;
    CHECK_NOTHROW(
      active_poller.add(s.server, ZMQ_POLLIN, [&count](short) { ++count; }));

    // client sends message
    CHECK_NOTHROW(s.client.send(zmq::message_t{"Hi"}));

    // wait for message and verify events
    CHECK(1 == active_poller.wait(std::chrono::milliseconds{500}));
    CHECK(1u == count);
}

TEST_CASE("wait on move constructed active_poller", "[active_poller]")
{
    server_client_setup s;
    CHECK_NOTHROW(s.client.send(zmq::message_t{"Hi"}));
    zmq::active_poller_t a;
    zmq::active_poller_t::handler_t handler;
    CHECK_NOTHROW(a.add(s.server, ZMQ_POLLIN, handler));
    zmq::active_poller_t b{std::move(a)};
    CHECK(1u == b.size());
    /// \todo the actual error code should be checked
    CHECK_THROWS_AS(a.wait(std::chrono::milliseconds{10}), zmq::error_t);
    CHECK(b.wait(std::chrono::milliseconds{-1}));
}

TEST_CASE("wait on move assigned active_poller", "[active_poller]")
{
    server_client_setup s;
    CHECK_NOTHROW(s.client.send(zmq::message_t{"Hi"}));
    zmq::active_poller_t a;
    zmq::active_poller_t::handler_t handler;
    CHECK_NOTHROW(a.add(s.server, ZMQ_POLLIN, handler));
    zmq::active_poller_t b;
    b = {std::move(a)};
    CHECK(1u == b.size());
    /// \todo the actual error code should be checked
    CHECK_THROWS_AS(a.wait(std::chrono::milliseconds{10}), zmq::error_t);
    CHECK(b.wait(std::chrono::milliseconds{-1}));
}

TEST_CASE("received on move constructed active_poller", "[active_poller]")
{
    // Setup server and client
    server_client_setup s;
    int count = 0;
    // Setup active_poller a
    zmq::active_poller_t a;
    CHECK_NOTHROW(a.add(s.server, ZMQ_POLLIN, [&count](short) { ++count; }));
    // client sends message
    CHECK_NOTHROW(s.client.send(zmq::message_t{"Hi"}));
    // wait for message and verify it is received
    CHECK(1 == a.wait(std::chrono::milliseconds{500}));
    CHECK(1u == count);
    // Move construct active_poller b
    zmq::active_poller_t b{std::move(a)};
    // client sends message again
    CHECK_NOTHROW(s.client.send(zmq::message_t{"Hi"}));
    // wait for message and verify it is received
    CHECK(1 == b.wait(std::chrono::milliseconds{500}));
    CHECK(2u == count);
}


TEST_CASE("remove from handler", "[active_poller]")
{
    constexpr auto ITER_NO = 10;

    // Setup servers and clients
    std::vector<server_client_setup> setup_list;
    for (auto i = 0; i < ITER_NO; ++i)
        setup_list.emplace_back(server_client_setup{});

    // Setup active_poller
    zmq::active_poller_t active_poller;
    int count = 0;
    for (auto i = 0; i < ITER_NO; ++i) {
        CHECK_NOTHROW(
          active_poller.add(setup_list[i].server, ZMQ_POLLIN, [&, i](short events) {
              CHECK(events == ZMQ_POLLIN);
              active_poller.remove(setup_list[ITER_NO - i - 1].server);
              CHECK((ITER_NO - i - 1) == active_poller.size());
          }));
        ++count;
    }
    CHECK(ITER_NO == active_poller.size());
    // Clients send messages
    for (auto &s : setup_list) {
        CHECK_NOTHROW(s.client.send(zmq::message_t{"Hi"}));
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
