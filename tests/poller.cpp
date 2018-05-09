#include <gtest/gtest.h>
#include <zmq.hpp>

#if defined(ZMQ_CPP11) && defined(ZMQ_BUILD_DRAFT_API)

#include <array>
#include <memory>

TEST(poller, create_destroy)
{
    zmq::poller_t poller;
    ASSERT_TRUE(poller.empty ());
}

static_assert(!std::is_copy_constructible<zmq::poller_t>::value, "poller_t should not be copy-constructible");
static_assert(!std::is_copy_assignable<zmq::poller_t>::value, "poller_t should not be copy-assignable");

TEST(poller, move_construct_empty)
{
    std::unique_ptr<zmq::poller_t> a {new zmq::poller_t};
    ASSERT_TRUE(a->empty ());
    zmq::poller_t b = std::move (*a);
    ASSERT_TRUE(b.empty ());
    ASSERT_EQ(0u, a->size ());
    ASSERT_EQ(0u, b.size ());
    a.reset ();
}

TEST(poller, move_assign_empty)
{
    std::unique_ptr<zmq::poller_t> a{new zmq::poller_t};
    ASSERT_TRUE(a->empty());
    zmq::poller_t b;
    ASSERT_TRUE(b.empty());
    b = std::move(*a);
    ASSERT_EQ(0u, a->size ());
    ASSERT_EQ(0u, b.size ());
    ASSERT_TRUE(a->empty());
    ASSERT_TRUE(b.empty());
    a.reset ();
}

TEST(poller, move_construct_non_empty)
{
    zmq::context_t context;
    zmq::socket_t socket{context, zmq::socket_type::router};

    std::unique_ptr<zmq::poller_t> a{new zmq::poller_t};
    a->add(socket, ZMQ_POLLIN, [](short) {});
    ASSERT_FALSE(a->empty ());
    ASSERT_EQ(1u, a->size ());
    zmq::poller_t b = std::move (*a);
    ASSERT_TRUE(a->empty ());
    ASSERT_EQ(0u, a->size ());
    ASSERT_FALSE(b.empty ());
    ASSERT_EQ(1u, b.size ());
    a.reset ();
}

TEST(poller, move_assign_non_empty)
{
    zmq::context_t context;
    zmq::socket_t socket{context, zmq::socket_type::router};

    std::unique_ptr<zmq::poller_t> a{new zmq::poller_t};
    a->add(socket, ZMQ_POLLIN, [](short) {});
    ASSERT_FALSE(a->empty());
    ASSERT_EQ(1u, a->size ());
    zmq::poller_t b;
    b = std::move(*a);
    ASSERT_TRUE(a->empty ());
    ASSERT_EQ(0u, a->size ());
    ASSERT_FALSE(b.empty ());
    ASSERT_EQ(1u, b.size ());
    a.reset ();
}

TEST(poller, add_handler)
{
    zmq::context_t context;
    zmq::socket_t socket{context, zmq::socket_type::router};
    zmq::poller_t poller;
    zmq::poller_t::handler_t handler;
    ASSERT_NO_THROW(poller.add(socket, ZMQ_POLLIN, handler));
}

TEST(poller, add_handler_invalid_events_type)
{
    zmq::context_t context;
    zmq::socket_t socket{context, zmq::socket_type::router};
    zmq::poller_t poller;
    zmq::poller_t::handler_t handler;
    short invalid_events_type = 2 << 10;
    ASSERT_NO_THROW(poller.add(socket, invalid_events_type, handler));
    ASSERT_FALSE(poller.empty ());
    ASSERT_EQ(1u, poller.size ());
}

TEST(poller, add_handler_twice_throws)
{
    zmq::context_t context;
    zmq::socket_t socket{context, zmq::socket_type::router};
    zmq::poller_t poller;
    zmq::poller_t::handler_t handler;
    poller.add(socket, ZMQ_POLLIN, handler);
    /// \todo the actual error code should be checked
    ASSERT_THROW(poller.add(socket, ZMQ_POLLIN, handler), zmq::error_t);
}

TEST(poller, wait_with_no_handlers_throws)
{
    zmq::poller_t poller;
    /// \todo the actual error code should be checked
    ASSERT_THROW(poller.wait(std::chrono::milliseconds{10}), zmq::error_t);
}

TEST(poller, remove_unregistered_throws)
{
    zmq::context_t context;
    zmq::socket_t socket{context, zmq::socket_type::router};
    zmq::poller_t poller;
    /// \todo the actual error code should be checked
    ASSERT_THROW(poller.remove(socket), zmq::error_t);
}

TEST(poller, remove_registered_empty)
{
    zmq::context_t context;
    zmq::socket_t socket{context, zmq::socket_type::router};
    zmq::poller_t poller;
    poller.add(socket, ZMQ_POLLIN, zmq::poller_t::handler_t{});
    ASSERT_NO_THROW(poller.remove(socket));
}

TEST(poller, remove_registered_non_empty)
{
    zmq::context_t context;
    zmq::socket_t socket{context, zmq::socket_type::router};
    zmq::poller_t poller;
    poller.add(socket, ZMQ_POLLIN, [](short) {});
    ASSERT_NO_THROW(poller.remove(socket));
}

namespace {

class loopback_ip4_binder
{
public:
    loopback_ip4_binder(zmq::socket_t &socket) { bind(socket); }
    std::string endpoint() { return endpoint_; }
private:
    // Helper function used in constructor
    // as Gtest allows ASSERT_* only in void returning functions
    // and constructor/destructor are not.
    void bind(zmq::socket_t &socket)
    {
        ASSERT_NO_THROW(socket.bind("tcp://127.0.0.1:*"));
        std::array<char, 100> endpoint{};
        size_t endpoint_size = endpoint.size();
        ASSERT_NO_THROW(socket.getsockopt(ZMQ_LAST_ENDPOINT, endpoint.data(),
                                          &endpoint_size));
        ASSERT_TRUE(endpoint_size < endpoint.size());
        endpoint_ = std::string{endpoint.data()};
    }
    std::string endpoint_;
};

struct server_client_setup
{
    server_client_setup ()
    {
        init ();
    }

    void init()
    {
        endpoint = loopback_ip4_binder {server}.endpoint ();
        ASSERT_NO_THROW (client.connect (endpoint));
    }

    zmq::poller_t::handler_t handler = [&](short e) {
        events = e;
    };

    zmq::context_t context;
    zmq::socket_t server {context, zmq::socket_type::router};
    zmq::socket_t client {context, zmq::socket_type::dealer};
    std::string endpoint;
    short events = 0;
};

} //namespace

TEST(poller, poll_basic)
{
    server_client_setup s;

    ASSERT_NO_THROW(s.client.send("Hi"));

    zmq::poller_t poller;
    bool message_received = false;
    zmq::poller_t::handler_t handler = [&message_received](short events) {
        ASSERT_TRUE(0 != (events & ZMQ_POLLIN));
        message_received = true;
    };
    ASSERT_NO_THROW(poller.add(s.server, ZMQ_POLLIN, handler));
    ASSERT_NO_THROW(poller.wait(std::chrono::milliseconds{-1}));
    ASSERT_TRUE(message_received);
}

/// \todo this contains multiple test cases that should be split up
TEST(poller, client_server)
{
    const std::string send_msg = "Hi";

    // Setup server and client
    server_client_setup s;

    // Setup poller
    zmq::poller_t poller;
    short events;
    zmq::poller_t::handler_t handler = [&](short e) {
        if (0 != (e & ZMQ_POLLIN)) {
            zmq::message_t zmq_msg;
            ASSERT_NO_THROW(s.server.recv(&zmq_msg)); // skip msg id
            ASSERT_NO_THROW(s.server.recv(&zmq_msg)); // get message
            std::string recv_msg(zmq_msg.data<char>(),
                                 zmq_msg.size());
            ASSERT_EQ(send_msg, recv_msg);
        } else if (0 != (e & ~ZMQ_POLLOUT)) {
            ASSERT_TRUE(false) << "Unexpected event type " << events;
        }
        events = e;
    };

    ASSERT_NO_THROW(poller.add(s.server, ZMQ_POLLIN, handler));

    // client sends message
    ASSERT_NO_THROW(s.client.send(send_msg));

    ASSERT_NO_THROW(poller.wait(std::chrono::milliseconds{-1}));
    ASSERT_EQ(events, ZMQ_POLLIN);

    // Re-add server socket with pollout flag
    ASSERT_NO_THROW(poller.remove(s.server));
    ASSERT_NO_THROW(poller.add(s.server, ZMQ_POLLIN | ZMQ_POLLOUT, handler));
    ASSERT_NO_THROW(poller.wait(std::chrono::milliseconds{-1}));
    ASSERT_EQ(events, ZMQ_POLLOUT);
}

TEST(poller, add_invalid_socket_throws)
{
    zmq::context_t context;
    zmq::poller_t poller;
    zmq::socket_t a {context, zmq::socket_type::router};
    zmq::socket_t b {std::move (a)};
    ASSERT_THROW (poller.add (a, ZMQ_POLLIN, zmq::poller_t::handler_t {}),
                  zmq::error_t);
}

TEST(poller, remove_invalid_socket_throws)
{
    zmq::context_t context;
    zmq::socket_t socket {context, zmq::socket_type::router};
    zmq::poller_t poller;
    ASSERT_NO_THROW (poller.add (socket, ZMQ_POLLIN, zmq::poller_t::handler_t {}));
    ASSERT_EQ (1u, poller.size ());
    std::vector<zmq::socket_t> sockets;
    sockets.emplace_back (std::move (socket));
    ASSERT_THROW (poller.remove (socket), zmq::error_t);
    ASSERT_EQ (1u, poller.size ());
}

TEST(poller, wait_on_added_empty_handler)
{
    server_client_setup s;
    ASSERT_NO_THROW (s.client.send ("Hi"));
    zmq::poller_t poller;
    zmq::poller_t::handler_t handler;
    ASSERT_NO_THROW (poller.add (s.server, ZMQ_POLLIN, handler));
    ASSERT_NO_THROW (poller.wait (std::chrono::milliseconds {-1}));
}

TEST(poller, modify_empty_throws)
{
    zmq::context_t context;
    zmq::socket_t socket {context, zmq::socket_type::push};
    zmq::poller_t poller;
    ASSERT_THROW (poller.modify (socket, ZMQ_POLLIN), zmq::error_t);
}

TEST(poller, modify_invalid_socket_throws)
{
    zmq::context_t context;
    zmq::socket_t a {context, zmq::socket_type::push};
    zmq::socket_t b {std::move (a)};
    zmq::poller_t poller;
    ASSERT_THROW (poller.modify (a, ZMQ_POLLIN), zmq::error_t);
}

TEST(poller, modify_not_added_throws)
{
    zmq::context_t context;
    zmq::socket_t a {context, zmq::socket_type::push};
    zmq::socket_t b {context, zmq::socket_type::push};
    zmq::poller_t poller;
    ASSERT_NO_THROW (poller.add (a, ZMQ_POLLIN, zmq::poller_t::handler_t {}));
    ASSERT_THROW (poller.modify (b, ZMQ_POLLIN), zmq::error_t);
}

TEST(poller, modify_simple)
{
    zmq::context_t context;
    zmq::socket_t a {context, zmq::socket_type::push};
    zmq::poller_t poller;
    ASSERT_NO_THROW (poller.add (a, ZMQ_POLLIN, zmq::poller_t::handler_t {}));
    ASSERT_NO_THROW (poller.modify (a, ZMQ_POLLIN|ZMQ_POLLOUT));
}

TEST(poller, poll_client_server)
{
    // Setup server and client
    server_client_setup s;

    // Setup poller
    zmq::poller_t poller;
    ASSERT_NO_THROW(poller.add(s.server, ZMQ_POLLIN, s.handler));

    // client sends message
    ASSERT_NO_THROW(s.client.send("Hi"));

    // wait for message and verify events
    ASSERT_NO_THROW(poller.wait(std::chrono::milliseconds{500}));
    ASSERT_TRUE(s.events == ZMQ_POLLIN);
    ASSERT_EQ(s.events, ZMQ_POLLIN);

    // Modify server socket with pollout flag
    ASSERT_NO_THROW(poller.modify(s.server, ZMQ_POLLIN | ZMQ_POLLOUT));
    ASSERT_NO_THROW(poller.wait(std::chrono::milliseconds{500}));
    ASSERT_EQ(s.events, ZMQ_POLLIN | ZMQ_POLLOUT);
}

TEST(poller, wait_one_return)
{
    // Setup server and client
    server_client_setup s;

    int count = 0;

    // Setup poller
    zmq::poller_t poller;
    ASSERT_NO_THROW(poller.add(s.server, ZMQ_POLLIN, [&count](short) {
        ++count;
    }));

    // client sends message
    ASSERT_NO_THROW(s.client.send("Hi"));

    // wait for message and verify events
    int result = poller.wait(std::chrono::milliseconds{500});
    ASSERT_EQ(count, result);
}

TEST(poller, wait_on_move_constructed_poller)
{
    server_client_setup s;
    ASSERT_NO_THROW (s.client.send ("Hi"));
    zmq::poller_t a;
    zmq::poller_t::handler_t handler;
    ASSERT_NO_THROW (a.add (s.server, ZMQ_POLLIN, handler));
    ASSERT_EQ(1u, a.size ());
    zmq::poller_t b {std::move (a)};
    ASSERT_EQ(1u, b.size ());
    ASSERT_NO_THROW (b.wait (std::chrono::milliseconds {-1}));
}

TEST(poller, wait_on_move_assign_poller)
{
    server_client_setup s;
    ASSERT_NO_THROW (s.client.send ("Hi"));
    zmq::poller_t a;
    zmq::poller_t::handler_t handler;
    ASSERT_NO_THROW (a.add (s.server, ZMQ_POLLIN, handler));
    ASSERT_EQ(1u, a.size ());
    zmq::poller_t b;
    ASSERT_EQ(0u, b.size ());
    b = {std::move (a)};
    ASSERT_EQ(1u, b.size ());
    ASSERT_NO_THROW (b.wait (std::chrono::milliseconds {-1}));
}

TEST(poller, received_on_move_construced_poller)
{
    // Setup server and client
    server_client_setup s;
    int count = 0;
    // Setup poller a
    zmq::poller_t a;
    ASSERT_NO_THROW(a.add(s.server, ZMQ_POLLIN, [&count](short) {
        ++count;
    }));
    // client sends message
    ASSERT_NO_THROW(s.client.send("Hi"));
    // wait for message and verify it is received
    a.wait(std::chrono::milliseconds{500});
    ASSERT_EQ(1u, count);
    // Move construct poller b
    zmq::poller_t b{std::move(a)};
    // client sends message again
    ASSERT_NO_THROW(s.client.send("Hi"));
    // wait for message and verify it is received
    b.wait(std::chrono::milliseconds{500});
    ASSERT_EQ(2u, count);
}


TEST(poller, remove_from_handler)
{
    constexpr auto ITER_NO = 10;

    // Setup servers and clients
    std::vector<server_client_setup> setup_list;
    for (auto i = 0; i < ITER_NO; ++i)
        setup_list.emplace_back (server_client_setup{});

    // Setup poller
    zmq::poller_t poller;
    for (auto i = 0; i < ITER_NO; ++i) {
        ASSERT_NO_THROW(poller.add(setup_list[i].server, ZMQ_POLLIN, [&,i](short events) {
            ASSERT_EQ(events, ZMQ_POLLIN);
            poller.remove(setup_list[ITER_NO-i-1].server);
            ASSERT_EQ(ITER_NO-i-1, poller.size());
        }));
    }
    ASSERT_EQ(ITER_NO, poller.size());
    // Clients send messages
    for (auto & s : setup_list) {
        ASSERT_NO_THROW(s.client.send("Hi"));
    }

    // Wait for all servers to receive a message
    for (auto & s : setup_list) {
        zmq::pollitem_t items [] = { { s.server, 0, ZMQ_POLLIN, 0 } };
        zmq::poll (&items [0], 1);
    }

    // Fire all handlers in one wait
    int count = poller.wait (std::chrono::milliseconds{-1});
    ASSERT_EQ(count, ITER_NO);
}

#endif
