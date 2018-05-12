#include <zmq_addon.hpp>

#include "testutil.hpp"

#if defined(ZMQ_CPP11) && defined(ZMQ_BUILD_DRAFT_API)

#include <array>
#include <memory>

TEST(active_poller, create_destroy)
{
    zmq::active_poller_t active_poller;
    ASSERT_TRUE(active_poller.empty());
}

static_assert(!std::is_copy_constructible<zmq::active_poller_t>::value,
              "active_active_poller_t should not be copy-constructible");
static_assert(!std::is_copy_assignable<zmq::active_poller_t>::value,
              "active_active_poller_t should not be copy-assignable");

TEST(active_poller, move_construct_empty)
{
    zmq::active_poller_t a;
    ASSERT_TRUE(a.empty());
    zmq::active_poller_t b = std::move(a);
    ASSERT_TRUE(b.empty());
    ASSERT_EQ(0u, a.size());
    ASSERT_EQ(0u, b.size());
}

TEST(active_poller, move_assign_empty)
{
    zmq::active_poller_t a;
    ASSERT_TRUE(a.empty());
    zmq::active_poller_t b;
    ASSERT_TRUE(b.empty());
    b = std::move(a);
    ASSERT_EQ(0u, a.size());
    ASSERT_EQ(0u, b.size());
    ASSERT_TRUE(a.empty());
    ASSERT_TRUE(b.empty());
}

TEST(active_poller, move_construct_non_empty)
{
    zmq::context_t context;
    zmq::socket_t socket{context, zmq::socket_type::router};

    zmq::active_poller_t a;
    a.add(socket, ZMQ_POLLIN, [](short) {});
    ASSERT_FALSE(a.empty());
    ASSERT_EQ(1u, a.size());
    zmq::active_poller_t b = std::move(a);
    ASSERT_TRUE(a.empty());
    ASSERT_EQ(0u, a.size());
    ASSERT_FALSE(b.empty());
    ASSERT_EQ(1u, b.size());
}

TEST(active_poller, move_assign_non_empty)
{
    zmq::context_t context;
    zmq::socket_t socket{context, zmq::socket_type::router};

    zmq::active_poller_t a;
    a.add(socket, ZMQ_POLLIN, [](short) {});
    ASSERT_FALSE(a.empty());
    ASSERT_EQ(1u, a.size());
    zmq::active_poller_t b;
    b = std::move(a);
    ASSERT_TRUE(a.empty());
    ASSERT_EQ(0u, a.size());
    ASSERT_FALSE(b.empty());
    ASSERT_EQ(1u, b.size());
}

TEST(active_poller, add_handler)
{
    zmq::context_t context;
    zmq::socket_t socket{context, zmq::socket_type::router};
    zmq::active_poller_t active_poller;
    zmq::active_poller_t::handler_t handler;
    ASSERT_NO_THROW(active_poller.add(socket, ZMQ_POLLIN, handler));
}

TEST(active_poller, add_handler_invalid_events_type)
{
    /// \todo is it good that this is accepted? should probably already be
    ///   checked by zmq_poller_add/modify in libzmq:
    ///   https://github.com/zeromq/libzmq/issues/3088
    zmq::context_t context;
    zmq::socket_t socket{context, zmq::socket_type::router};
    zmq::active_poller_t active_poller;
    zmq::active_poller_t::handler_t handler;
    short invalid_events_type = 2 << 10;
    ASSERT_NO_THROW(active_poller.add(socket, invalid_events_type, handler));
    ASSERT_FALSE(active_poller.empty());
    ASSERT_EQ(1u, active_poller.size());
}

TEST(active_poller, add_handler_twice_throws)
{
    zmq::context_t context;
    zmq::socket_t socket{context, zmq::socket_type::router};
    zmq::active_poller_t active_poller;
    zmq::active_poller_t::handler_t handler;
    active_poller.add(socket, ZMQ_POLLIN, handler);
    /// \todo the actual error code should be checked
    ASSERT_THROW(active_poller.add(socket, ZMQ_POLLIN, handler), zmq::error_t);
}

TEST(active_poller, wait_with_no_handlers_throws)
{
    zmq::active_poller_t active_poller;
    /// \todo the actual error code should be checked
    ASSERT_THROW(active_poller.wait(std::chrono::milliseconds{10}), zmq::error_t);
}

TEST(active_poller, remove_unregistered_throws)
{
    zmq::context_t context;
    zmq::socket_t socket{context, zmq::socket_type::router};
    zmq::active_poller_t active_poller;
    /// \todo the actual error code should be checked
    ASSERT_THROW(active_poller.remove(socket), zmq::error_t);
}

TEST(active_poller, remove_registered_empty)
{
    zmq::context_t context;
    zmq::socket_t socket{context, zmq::socket_type::router};
    zmq::active_poller_t active_poller;
    active_poller.add(socket, ZMQ_POLLIN, zmq::active_poller_t::handler_t{});
    ASSERT_NO_THROW(active_poller.remove(socket));
}

TEST(active_poller, remove_registered_non_empty)
{
    zmq::context_t context;
    zmq::socket_t socket{context, zmq::socket_type::router};
    zmq::active_poller_t active_poller;
    active_poller.add(socket, ZMQ_POLLIN, [](short) {});
    ASSERT_NO_THROW(active_poller.remove(socket));
}

namespace
{
struct server_client_setup : common_server_client_setup
{
    zmq::active_poller_t::handler_t handler = [&](short e) { events = e; };

    short events = 0;
};
}

TEST(active_poller, poll_basic)
{
    server_client_setup s;

    ASSERT_NO_THROW(s.client.send("Hi"));

    zmq::active_poller_t active_poller;
    bool message_received = false;
    zmq::active_poller_t::handler_t handler = [&message_received](short events) {
        ASSERT_TRUE(0 != (events & ZMQ_POLLIN));
        message_received = true;
    };
    ASSERT_NO_THROW(active_poller.add(s.server, ZMQ_POLLIN, handler));
    ASSERT_EQ(1, active_poller.wait(std::chrono::milliseconds{-1}));
    ASSERT_TRUE(message_received);
}

/// \todo this contains multiple test cases that should be split up
TEST(active_poller, client_server)
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
            ASSERT_NO_THROW(s.server.recv(&zmq_msg)); // get message
            std::string recv_msg(zmq_msg.data<char>(), zmq_msg.size());
            ASSERT_EQ(send_msg, recv_msg);
        } else if (0 != (e & ~ZMQ_POLLOUT)) {
            ASSERT_TRUE(false) << "Unexpected event type " << events;
        }
        events = e;
    };

    ASSERT_NO_THROW(active_poller.add(s.server, ZMQ_POLLIN, handler));

    // client sends message
    ASSERT_NO_THROW(s.client.send(send_msg));

    ASSERT_EQ(1, active_poller.wait(std::chrono::milliseconds{-1}));
    ASSERT_EQ(events, ZMQ_POLLIN);

    // Re-add server socket with pollout flag
    ASSERT_NO_THROW(active_poller.remove(s.server));
    ASSERT_NO_THROW(active_poller.add(s.server, ZMQ_POLLIN | ZMQ_POLLOUT, handler));
    ASSERT_EQ(1, active_poller.wait(std::chrono::milliseconds{-1}));
    ASSERT_EQ(events, ZMQ_POLLOUT);
}

TEST(active_poller, add_invalid_socket_throws)
{
    zmq::context_t context;
    zmq::active_poller_t active_poller;
    zmq::socket_t a{context, zmq::socket_type::router};
    zmq::socket_t b{std::move(a)};
    ASSERT_THROW(active_poller.add(a, ZMQ_POLLIN, zmq::active_poller_t::handler_t{}),
                 zmq::error_t);
}

TEST(active_poller, remove_invalid_socket_throws)
{
    zmq::context_t context;
    zmq::socket_t socket{context, zmq::socket_type::router};
    zmq::active_poller_t active_poller;
    ASSERT_NO_THROW(
      active_poller.add(socket, ZMQ_POLLIN, zmq::active_poller_t::handler_t{}));
    ASSERT_EQ(1u, active_poller.size());
    std::vector<zmq::socket_t> sockets;
    sockets.emplace_back(std::move(socket));
    ASSERT_THROW(active_poller.remove(socket), zmq::error_t);
    ASSERT_EQ(1u, active_poller.size());
}

TEST(active_poller, wait_on_added_empty_handler)
{
    server_client_setup s;
    ASSERT_NO_THROW(s.client.send("Hi"));
    zmq::active_poller_t active_poller;
    zmq::active_poller_t::handler_t handler;
    ASSERT_NO_THROW(active_poller.add(s.server, ZMQ_POLLIN, handler));
    ASSERT_NO_THROW(active_poller.wait(std::chrono::milliseconds{-1}));
}

TEST(active_poller, modify_empty_throws)
{
    zmq::context_t context;
    zmq::socket_t socket{context, zmq::socket_type::push};
    zmq::active_poller_t active_poller;
    ASSERT_THROW(active_poller.modify(socket, ZMQ_POLLIN), zmq::error_t);
}

TEST(active_poller, modify_invalid_socket_throws)
{
    zmq::context_t context;
    zmq::socket_t a{context, zmq::socket_type::push};
    zmq::socket_t b{std::move(a)};
    zmq::active_poller_t active_poller;
    ASSERT_THROW(active_poller.modify(a, ZMQ_POLLIN), zmq::error_t);
}

TEST(active_poller, modify_not_added_throws)
{
    zmq::context_t context;
    zmq::socket_t a{context, zmq::socket_type::push};
    zmq::socket_t b{context, zmq::socket_type::push};
    zmq::active_poller_t active_poller;
    ASSERT_NO_THROW(
      active_poller.add(a, ZMQ_POLLIN, zmq::active_poller_t::handler_t{}));
    ASSERT_THROW(active_poller.modify(b, ZMQ_POLLIN), zmq::error_t);
}

TEST(active_poller, modify_simple)
{
    zmq::context_t context;
    zmq::socket_t a{context, zmq::socket_type::push};
    zmq::active_poller_t active_poller;
    ASSERT_NO_THROW(
      active_poller.add(a, ZMQ_POLLIN, zmq::active_poller_t::handler_t{}));
    ASSERT_NO_THROW(active_poller.modify(a, ZMQ_POLLIN | ZMQ_POLLOUT));
}

TEST(active_poller, poll_client_server)
{
    // Setup server and client
    server_client_setup s;

    // Setup active_poller
    zmq::active_poller_t active_poller;
    ASSERT_NO_THROW(active_poller.add(s.server, ZMQ_POLLIN, s.handler));

    // client sends message
    ASSERT_NO_THROW(s.client.send("Hi"));

    // wait for message and verify events
    ASSERT_NO_THROW(active_poller.wait(std::chrono::milliseconds{500}));
    ASSERT_EQ(s.events, ZMQ_POLLIN);

    // Modify server socket with pollout flag
    ASSERT_NO_THROW(active_poller.modify(s.server, ZMQ_POLLIN | ZMQ_POLLOUT));
    ASSERT_EQ(1, active_poller.wait(std::chrono::milliseconds{500}));
    ASSERT_EQ(s.events, ZMQ_POLLIN | ZMQ_POLLOUT);
}

TEST(active_poller, wait_one_return)
{
    // Setup server and client
    server_client_setup s;

    int count = 0;

    // Setup active_poller
    zmq::active_poller_t active_poller;
    ASSERT_NO_THROW(
      active_poller.add(s.server, ZMQ_POLLIN, [&count](short) { ++count; }));

    // client sends message
    ASSERT_NO_THROW(s.client.send("Hi"));

    // wait for message and verify events
    ASSERT_EQ(1, active_poller.wait(std::chrono::milliseconds{500}));
    ASSERT_EQ(1u, count);
}

TEST(active_poller, wait_on_move_constructed_active_poller)
{
    server_client_setup s;
    ASSERT_NO_THROW(s.client.send("Hi"));
    zmq::active_poller_t a;
    zmq::active_poller_t::handler_t handler;
    ASSERT_NO_THROW(a.add(s.server, ZMQ_POLLIN, handler));
    zmq::active_poller_t b{std::move(a)};
    ASSERT_EQ(1u, b.size());
    /// \todo the actual error code should be checked
    ASSERT_THROW(a.wait(std::chrono::milliseconds{10}), zmq::error_t);
    ASSERT_TRUE(b.wait(std::chrono::milliseconds{-1}));
}

TEST(active_poller, wait_on_move_assigned_active_poller)
{
    server_client_setup s;
    ASSERT_NO_THROW(s.client.send("Hi"));
    zmq::active_poller_t a;
    zmq::active_poller_t::handler_t handler;
    ASSERT_NO_THROW(a.add(s.server, ZMQ_POLLIN, handler));
    zmq::active_poller_t b;
    b = {std::move(a)};
    ASSERT_EQ(1u, b.size());
    /// \todo the actual error code should be checked
    ASSERT_THROW(a.wait(std::chrono::milliseconds{10}), zmq::error_t);
    ASSERT_TRUE(b.wait(std::chrono::milliseconds{-1}));
}

TEST(active_poller, received_on_move_constructed_active_poller)
{
    // Setup server and client
    server_client_setup s;
    int count = 0;
    // Setup active_poller a
    zmq::active_poller_t a;
    ASSERT_NO_THROW(a.add(s.server, ZMQ_POLLIN, [&count](short) { ++count; }));
    // client sends message
    ASSERT_NO_THROW(s.client.send("Hi"));
    // wait for message and verify it is received
    ASSERT_EQ(1, a.wait(std::chrono::milliseconds{500}));
    ASSERT_EQ(1u, count);
    // Move construct active_poller b
    zmq::active_poller_t b{std::move(a)};
    // client sends message again
    ASSERT_NO_THROW(s.client.send("Hi"));
    // wait for message and verify it is received
    ASSERT_EQ(1, b.wait(std::chrono::milliseconds{500}));
    ASSERT_EQ(2u, count);
}


TEST(active_poller, remove_from_handler)
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
        ASSERT_NO_THROW(
          active_poller.add(setup_list[i].server, ZMQ_POLLIN, [&, i](short events) {
              ASSERT_EQ(events, ZMQ_POLLIN);
              active_poller.remove(setup_list[ITER_NO - i - 1].server);
              ASSERT_EQ(ITER_NO - i - 1, active_poller.size());
          }));
        ++count;
    }
    ASSERT_EQ(ITER_NO, active_poller.size());
    // Clients send messages
    for (auto &s : setup_list) {
        ASSERT_NO_THROW(s.client.send("Hi"));
    }

    // Wait for all servers to receive a message
    for (auto &s : setup_list) {
        zmq::pollitem_t items[] = {{s.server, 0, ZMQ_POLLIN, 0}};
        zmq::poll(&items[0], 1);
    }

    // Fire all handlers in one wait
    ASSERT_EQ(ITER_NO, active_poller.wait(std::chrono::milliseconds{-1}));
    ASSERT_EQ(ITER_NO, count);
}

#endif
