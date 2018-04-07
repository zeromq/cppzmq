#include <gtest/gtest.h>
#include <zmq.hpp>

#if defined(ZMQ_CPP11) && defined(ZMQ_BUILD_DRAFT_API)

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
    std::function<void()> handler = [](){};
    ASSERT_NO_THROW(poller.add(socket, ZMQ_POLLIN, handler));
}

TEST(poller, add_handler_invalid_events_type)
{
    zmq::context_t context;
    zmq::socket_t socket{context, zmq::socket_type::router};
    zmq::poller_t poller;
    std::function<void()> handler = [](){};
    short invalid_events_type = 2 << 10;
    ASSERT_NO_THROW(poller.add(socket, invalid_events_type, handler));
}

TEST(poller, add_handler_twice_throws)
{
    zmq::context_t context;
    zmq::socket_t socket{context, zmq::socket_type::router};
    zmq::poller_t poller;
    std::function<void()> handler = [](){};
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
    std::function<void()> handler = [](){};
    poller.add(socket, ZMQ_POLLIN, handler);
    ASSERT_NO_THROW(poller.remove(socket));
}

TEST(poller, poll_basic)
{
    zmq::context_t context;

    zmq::socket_t vent{context, zmq::socket_type::push};
    ASSERT_NO_THROW(vent.bind("tcp://127.0.0.1:*"));

    zmq::socket_t sink{context, zmq::socket_type::pull};
    // TODO: this should be simpler...
    std::array<char, 100> endpoint{};
    size_t endpoint_size = endpoint.size();
    ASSERT_NO_THROW(vent.getsockopt(ZMQ_LAST_ENDPOINT, endpoint.data(),
                                    &endpoint_size));
    ASSERT_TRUE(endpoint_size < endpoint.size());
    ASSERT_NO_THROW(sink.connect(endpoint.data()));

    std::string message = "H";

    // TODO: send overload for string would be handy.
    ASSERT_NO_THROW(vent.send(std::begin(message), std::end(message)));

    zmq::poller_t poller;
    bool message_received = false;
    std::function<void()> handler = [&message_received]() {
        message_received = true;
    };
    ASSERT_NO_THROW(poller.add(sink, ZMQ_POLLIN, handler));
    ASSERT_NO_THROW(poller.wait(std::chrono::milliseconds{-1}));
    ASSERT_TRUE(message_received);
}

#endif
