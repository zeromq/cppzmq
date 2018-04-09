#include <gtest/gtest.h>
#include <zmq.hpp>

#if defined(ZMQ_CPP11) && defined(ZMQ_BUILD_DRAFT_API)

#include <array>

 TEST(poller, create_destroy)
 {
    zmq::poller_t poller;
}

TEST(poller, move_contruct)
{
    zmq::poller_t poller;
    zmq::poller_t poller_move {std::move (poller)};
}

TEST(poller, move_assign)
{
    zmq::poller_t poller;
    zmq::poller_t poller_assign;
    poller_assign = std::move (poller);
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
}

TEST(poller, add_handler_twice_throws)
{
    zmq::context_t context;
    zmq::socket_t socket{context, zmq::socket_type::router};
    zmq::poller_t poller;
    zmq::poller_t::handler_t handler;
    poller.add(socket, ZMQ_POLLIN, handler);
    ASSERT_THROW(poller.add(socket, ZMQ_POLLIN, handler), zmq::error_t);
}

TEST(poller, wait_with_no_handlers_throws)
{
    zmq::poller_t poller;
    ASSERT_THROW(poller.wait(std::chrono::milliseconds{10}), zmq::error_t);
}

TEST(poller, remove_unregistered_throws)
{
    zmq::context_t context;
    zmq::socket_t socket{context, zmq::socket_type::router};
    zmq::poller_t poller;
    ASSERT_THROW(poller.remove(socket), zmq::error_t);
}

TEST(poller, remove_registered)
{
    zmq::context_t context;
    zmq::socket_t socket{context, zmq::socket_type::router};
    zmq::poller_t poller;
    zmq::poller_t::handler_t handler;
    poller.add(socket, ZMQ_POLLIN, handler);
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
    // as Gtest allows only void returning functions
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

} //namespace

TEST(poller, poll_basic)
{
    zmq::context_t context;

    zmq::socket_t vent{context, zmq::socket_type::push};
    auto endpoint = loopback_ip4_binder(vent).endpoint();

    zmq::socket_t sink{context, zmq::socket_type::pull};
    ASSERT_NO_THROW(sink.connect(endpoint));

    std::string message = "H";

    // TODO: send overload for string would be handy.
    ASSERT_NO_THROW(vent.send(std::begin(message), std::end(message)));

    zmq::poller_t poller;
    bool message_received = false;
    zmq::poller_t::handler_t handler = [&message_received](short events) {
        ASSERT_TRUE(0 != (events & ZMQ_POLLIN));
        message_received = true;
    };
    ASSERT_NO_THROW(poller.add(sink, ZMQ_POLLIN, handler));
    ASSERT_NO_THROW(poller.wait(std::chrono::milliseconds{-1}));
    ASSERT_TRUE(message_received);
}

TEST(poller, client_server)
{
    zmq::context_t context;
    std::string send_msg = "Hi";

    // Setup server
    zmq::socket_t server{context, zmq::socket_type::router};
    auto endpoint = loopback_ip4_binder(server).endpoint();

    // Setup poller
    zmq::poller_t poller;
    bool got_pollin = false;
    bool got_pollout = false;
    zmq::poller_t::handler_t handler = [&](short events) {
        if (0 != (events & ZMQ_POLLIN)) {
            zmq::message_t zmq_msg;
            ASSERT_NO_THROW(server.recv(&zmq_msg)); // skip msg id
            ASSERT_NO_THROW(server.recv(&zmq_msg)); // get message
            std::string recv_msg(zmq_msg.data<char>(),
                                 zmq_msg.size());
            ASSERT_EQ(send_msg, recv_msg);
            got_pollin = true;
        } else if (0 != (events & ZMQ_POLLOUT)) {
            got_pollout = true;
        } else {
            ASSERT_TRUE(false) << "Unexpected event type " << events;
        }
    };
    ASSERT_NO_THROW(poller.add(server, ZMQ_POLLIN, handler));

    // Setup client and send message
    zmq::socket_t client{context, zmq::socket_type::dealer};
    ASSERT_NO_THROW(client.connect(endpoint));
    ASSERT_NO_THROW(client.send(std::begin(send_msg), std::end(send_msg)));

    ASSERT_NO_THROW(poller.wait(std::chrono::milliseconds{-1}));
    ASSERT_TRUE(got_pollin);
    ASSERT_FALSE(got_pollout);

    // Re-add server socket with pollout flag
    ASSERT_NO_THROW(poller.remove(server));
    ASSERT_NO_THROW(poller.add(server, ZMQ_POLLIN | ZMQ_POLLOUT, handler));
    ASSERT_NO_THROW(poller.wait(std::chrono::milliseconds{-1}));
    ASSERT_TRUE(got_pollout);
}

#endif
