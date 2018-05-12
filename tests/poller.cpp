#include "testutil.hpp"

#if defined(ZMQ_CPP11) && defined(ZMQ_BUILD_DRAFT_API)

#include <array>
#include <memory>

TEST(poller, create_destroy)
{
    zmq::poller_t<> poller;
}

static_assert(!std::is_copy_constructible<zmq::poller_t<>>::value,
              "poller_t should not be copy-constructible");
static_assert(!std::is_copy_assignable<zmq::poller_t<>>::value,
              "poller_t should not be copy-assignable");

TEST(poller, move_construct_empty)
{
    zmq::poller_t<> a;
    zmq::poller_t<> b = std::move(a);
}

TEST(poller, move_assign_empty)
{
    zmq::poller_t<> a;
    zmq::poller_t<> b;
    b = std::move(a);
}

TEST(poller, move_construct_non_empty)
{
    zmq::context_t context;
    zmq::socket_t socket{context, zmq::socket_type::router};

    zmq::poller_t<> a;
    a.add(socket, ZMQ_POLLIN, nullptr);
    zmq::poller_t<> b = std::move(a);
}

TEST(poller, move_assign_non_empty)
{
    zmq::context_t context;
    zmq::socket_t socket{context, zmq::socket_type::router};

    zmq::poller_t<> a;
    a.add(socket, ZMQ_POLLIN, nullptr);
    zmq::poller_t<> b;
    b = std::move(a);
}

TEST(poller, add_nullptr)
{
    zmq::context_t context;
    zmq::socket_t socket{context, zmq::socket_type::router};
    zmq::poller_t<> poller;
    ASSERT_NO_THROW(poller.add(socket, ZMQ_POLLIN, nullptr));
}

TEST(poller, add_non_nullptr)
{
    zmq::context_t context;
    zmq::socket_t socket{context, zmq::socket_type::router};
    zmq::poller_t<> poller;
    int i;
    ASSERT_NO_THROW(poller.add(socket, ZMQ_POLLIN, &i));
}

TEST(poller, add_handler_invalid_events_type)
{
    /// \todo is it good that this is accepted? should probably already be
    ///   checked by zmq_poller_add/modify in libzmq:
    ///   https://github.com/zeromq/libzmq/issues/3088
    zmq::context_t context;
    zmq::socket_t socket{context, zmq::socket_type::router};
    zmq::poller_t<> poller;
    short invalid_events_type = 2 << 10;
    ASSERT_NO_THROW(poller.add(socket, invalid_events_type, nullptr));
}

TEST(poller, add_handler_twice_throws)
{
    zmq::context_t context;
    zmq::socket_t socket{context, zmq::socket_type::router};
    zmq::poller_t<> poller;
    poller.add(socket, ZMQ_POLLIN, nullptr);
    /// \todo the actual error code should be checked
    ASSERT_THROW(poller.add(socket, ZMQ_POLLIN, nullptr), zmq::error_t);
}

TEST(poller, wait_with_no_handlers_throws)
{
    zmq::poller_t<> poller;
    std::vector<zmq_poller_event_t> events;
    /// \todo the actual error code should be checked
    ASSERT_THROW(poller.wait_all(events, std::chrono::milliseconds{10}),
                 zmq::error_t);
}

TEST(poller, remove_unregistered_throws)
{
    zmq::context_t context;
    zmq::socket_t socket{context, zmq::socket_type::router};
    zmq::poller_t<> poller;
    /// \todo the actual error code should be checked
    ASSERT_THROW(poller.remove(socket), zmq::error_t);
}

TEST(poller, remove_registered_empty)
{
    zmq::context_t context;
    zmq::socket_t socket{context, zmq::socket_type::router};
    zmq::poller_t<> poller;
    poller.add(socket, ZMQ_POLLIN, nullptr);
    ASSERT_NO_THROW(poller.remove(socket));
}

TEST(poller, remove_registered_non_empty)
{
    zmq::context_t context;
    zmq::socket_t socket{context, zmq::socket_type::router};
    zmq::poller_t<> poller;
    poller.add(socket, ZMQ_POLLIN, nullptr);
    ASSERT_NO_THROW(poller.remove(socket));
}

TEST(poller, poll_basic)
{
    common_server_client_setup s;

    ASSERT_NO_THROW(s.client.send("Hi"));

    zmq::poller_t<int> poller;
    std::vector<zmq_poller_event_t> events{1};
    int i = 0;
    ASSERT_NO_THROW(poller.add(s.server, ZMQ_POLLIN, &i));
    ASSERT_EQ(1, poller.wait_all(events, std::chrono::milliseconds{-1}));
    ASSERT_EQ(s.server, events[0].socket);
    ASSERT_EQ(&i, events[0].user_data);
}

TEST(poller, add_invalid_socket_throws)
{
    zmq::context_t context;
    zmq::poller_t<> poller;
    zmq::socket_t a{context, zmq::socket_type::router};
    zmq::socket_t b{std::move(a)};
    ASSERT_THROW(poller.add(a, ZMQ_POLLIN, nullptr), zmq::error_t);
}

TEST(poller, remove_invalid_socket_throws)
{
    zmq::context_t context;
    zmq::socket_t socket{context, zmq::socket_type::router};
    zmq::poller_t<> poller;
    ASSERT_NO_THROW(poller.add(socket, ZMQ_POLLIN, nullptr));
    std::vector<zmq::socket_t> sockets;
    sockets.emplace_back(std::move(socket));
    ASSERT_THROW(poller.remove(socket), zmq::error_t);
    ASSERT_NO_THROW(poller.remove(sockets[0]));
}

TEST(poller, modify_empty_throws)
{
    zmq::context_t context;
    zmq::socket_t socket{context, zmq::socket_type::push};
    zmq::poller_t<> poller;
    ASSERT_THROW(poller.modify(socket, ZMQ_POLLIN), zmq::error_t);
}

TEST(poller, modify_invalid_socket_throws)
{
    zmq::context_t context;
    zmq::socket_t a{context, zmq::socket_type::push};
    zmq::socket_t b{std::move(a)};
    zmq::poller_t<> poller;
    ASSERT_THROW(poller.modify(a, ZMQ_POLLIN), zmq::error_t);
}

TEST(poller, modify_not_added_throws)
{
    zmq::context_t context;
    zmq::socket_t a{context, zmq::socket_type::push};
    zmq::socket_t b{context, zmq::socket_type::push};
    zmq::poller_t<> poller;
    ASSERT_NO_THROW(poller.add(a, ZMQ_POLLIN, nullptr));
    ASSERT_THROW(poller.modify(b, ZMQ_POLLIN), zmq::error_t);
}

TEST(poller, modify_simple)
{
    zmq::context_t context;
    zmq::socket_t a{context, zmq::socket_type::push};
    zmq::poller_t<> poller;
    ASSERT_NO_THROW(poller.add(a, ZMQ_POLLIN, nullptr));
    ASSERT_NO_THROW(poller.modify(a, ZMQ_POLLIN | ZMQ_POLLOUT));
}

TEST(poller, poll_client_server)
{
    // Setup server and client
    common_server_client_setup s;

    // Setup poller
    zmq::poller_t<> poller;
    ASSERT_NO_THROW(poller.add(s.server, ZMQ_POLLIN, s.server));

    // client sends message
    ASSERT_NO_THROW(s.client.send("Hi"));

    // wait for message and verify events
    std::vector<zmq_poller_event_t> events(1);
    ASSERT_EQ(1, poller.wait_all(events, std::chrono::milliseconds{500}));
    ASSERT_EQ(ZMQ_POLLIN, events[0].events);

    // Modify server socket with pollout flag
    ASSERT_NO_THROW(poller.modify(s.server, ZMQ_POLLIN | ZMQ_POLLOUT));
    ASSERT_EQ(1, poller.wait_all(events, std::chrono::milliseconds{500}));
    ASSERT_EQ(ZMQ_POLLIN | ZMQ_POLLOUT, events[0].events);
}

TEST(poller, wait_one_return)
{
    // Setup server and client
    common_server_client_setup s;

    // Setup poller
    zmq::poller_t<> poller;
    ASSERT_NO_THROW(poller.add(s.server, ZMQ_POLLIN, nullptr));

    // client sends message
    ASSERT_NO_THROW(s.client.send("Hi"));

    // wait for message and verify events
    std::vector<zmq_poller_event_t> events(1);
    ASSERT_EQ(1, poller.wait_all(events, std::chrono::milliseconds{500}));
}

TEST(poller, wait_on_move_constructed_poller)
{
    common_server_client_setup s;
    ASSERT_NO_THROW(s.client.send("Hi"));
    zmq::poller_t<> a;
    ASSERT_NO_THROW(a.add(s.server, ZMQ_POLLIN, nullptr));
    zmq::poller_t<> b{std::move(a)};
    std::vector<zmq_poller_event_t> events(1);
    /// \todo the actual error code should be checked
    ASSERT_THROW(a.wait_all(events, std::chrono::milliseconds{10}), zmq::error_t);
    ASSERT_EQ(1, b.wait_all(events, std::chrono::milliseconds{-1}));
}

TEST(poller, wait_on_move_assigned_poller)
{
    common_server_client_setup s;
    ASSERT_NO_THROW(s.client.send("Hi"));
    zmq::poller_t<> a;
    ASSERT_NO_THROW(a.add(s.server, ZMQ_POLLIN, nullptr));
    zmq::poller_t<> b;
    b = {std::move(a)};
    /// \todo the actual error code should be checked
    std::vector<zmq_poller_event_t> events(1);
    ASSERT_THROW(a.wait_all(events, std::chrono::milliseconds{10}), zmq::error_t);
    ASSERT_EQ(1, b.wait_all(events, std::chrono::milliseconds{-1}));
}

TEST(poller, remove_from_handler)
{
    constexpr auto ITER_NO = 10;

    // Setup servers and clients
    std::vector<common_server_client_setup> setup_list;
    for (auto i = 0; i < ITER_NO; ++i)
        setup_list.emplace_back(common_server_client_setup{});

    // Setup poller
    zmq::poller_t<> poller;
    for (auto i = 0; i < ITER_NO; ++i) {
        ASSERT_NO_THROW(poller.add(setup_list[i].server, ZMQ_POLLIN, nullptr));
    }
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
    std::vector<zmq_poller_event_t> events(ITER_NO);
    ASSERT_EQ(ITER_NO, poller.wait_all(events, std::chrono::milliseconds{-1}));
}

#endif
