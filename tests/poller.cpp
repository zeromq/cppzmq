#include "testutil.hpp"

#if defined(ZMQ_CPP11) && defined(ZMQ_BUILD_DRAFT_API)

#include <array>
#include <memory>

TEST_CASE("poller create destroy", "[poller]")
{
    zmq::poller_t<> poller;
}

static_assert(!std::is_copy_constructible<zmq::poller_t<>>::value,
              "poller_t should not be copy-constructible");
static_assert(!std::is_copy_assignable<zmq::poller_t<>>::value,
              "poller_t should not be copy-assignable");

TEST_CASE("poller move construct empty", "[poller]")
{
    zmq::poller_t<> a;
    zmq::poller_t<> b = std::move(a);
}

TEST_CASE("poller move assign empty", "[poller]")
{
    zmq::poller_t<> a;
    zmq::poller_t<> b;
    b = std::move(a);
}

TEST_CASE("poller move construct non empty", "[poller]")
{
    zmq::context_t context;
    zmq::socket_t socket{context, zmq::socket_type::router};

    zmq::poller_t<> a;
    a.add(socket, ZMQ_POLLIN, nullptr);
    zmq::poller_t<> b = std::move(a);
}

TEST_CASE("poller move assign non empty", "[poller]")
{
    zmq::context_t context;
    zmq::socket_t socket{context, zmq::socket_type::router};

    zmq::poller_t<> a;
    a.add(socket, ZMQ_POLLIN, nullptr);
    zmq::poller_t<> b;
    b = std::move(a);
}

TEST_CASE("poller add nullptr", "[poller]")
{
    zmq::context_t context;
    zmq::socket_t socket{context, zmq::socket_type::router};
    zmq::poller_t<> poller;
    CHECK_NOTHROW(poller.add(socket, ZMQ_POLLIN, nullptr));
}

TEST_CASE("poller add non nullptr", "[poller]")
{
    zmq::context_t context;
    zmq::socket_t socket{context, zmq::socket_type::router};
    zmq::poller_t<> poller;
    int i;
    CHECK_NOTHROW(poller.add(socket, ZMQ_POLLIN, &i));
}

#if ZMQ_VERSION >= ZMQ_MAKE_VERSION(4, 3, 0)
// this behaviour was added by https://github.com/zeromq/libzmq/pull/3100
TEST_CASE("poller add handler invalid events type", "[poller]")
{
    zmq::context_t context;
    zmq::socket_t socket{context, zmq::socket_type::router};
    zmq::poller_t<> poller;
    short invalid_events_type = 2 << 10;
    CHECK_THROWS_AS(poller.add(socket, invalid_events_type, nullptr), zmq::error_t);
}
#endif

TEST_CASE("poller add handler twice throws", "[poller]")
{
    zmq::context_t context;
    zmq::socket_t socket{context, zmq::socket_type::router};
    zmq::poller_t<> poller;
    poller.add(socket, ZMQ_POLLIN, nullptr);
    /// \todo the actual error code should be checked
    CHECK_THROWS_AS(poller.add(socket, ZMQ_POLLIN, nullptr), zmq::error_t);
}

TEST_CASE("poller wait with no handlers throws", "[poller]")
{
    zmq::poller_t<> poller;
    std::vector<zmq_poller_event_t> events;
    /// \todo the actual error code should be checked
    CHECK_THROWS_AS(poller.wait_all(events, std::chrono::milliseconds{10}),
                 zmq::error_t);
}

TEST_CASE("poller remove unregistered throws", "[poller]")
{
    zmq::context_t context;
    zmq::socket_t socket{context, zmq::socket_type::router};
    zmq::poller_t<> poller;
    /// \todo the actual error code should be checked
    CHECK_THROWS_AS(poller.remove(socket), zmq::error_t);
}

TEST_CASE("poller remove registered empty", "[poller]")
{
    zmq::context_t context;
    zmq::socket_t socket{context, zmq::socket_type::router};
    zmq::poller_t<> poller;
    poller.add(socket, ZMQ_POLLIN, nullptr);
    CHECK_NOTHROW(poller.remove(socket));
}

TEST_CASE("poller remove registered non empty", "[poller]")
{
    zmq::context_t context;
    zmq::socket_t socket{context, zmq::socket_type::router};
    zmq::poller_t<> poller;
    int empty{};
    poller.add(socket, ZMQ_POLLIN, &empty);
    CHECK_NOTHROW(poller.remove(socket));
}

TEST_CASE("poller poll basic", "[poller]")
{
    common_server_client_setup s;

    CHECK_NOTHROW(s.client.send(zmq::message_t{"Hi"}));

    zmq::poller_t<int> poller;
    std::vector<zmq_poller_event_t> events{1};
    int i = 0;
    CHECK_NOTHROW(poller.add(s.server, ZMQ_POLLIN, &i));
    CHECK(1 == poller.wait_all(events, std::chrono::milliseconds{-1}));
    CHECK(s.server == events[0].socket);
    CHECK(&i == events[0].user_data);
}

TEST_CASE("poller add invalid socket throws", "[poller]")
{
    zmq::context_t context;
    zmq::poller_t<> poller;
    zmq::socket_t a{context, zmq::socket_type::router};
    zmq::socket_t b{std::move(a)};
    CHECK_THROWS_AS(poller.add(a, ZMQ_POLLIN, nullptr), zmq::error_t);
}

TEST_CASE("poller remove invalid socket throws", "[poller]")
{
    zmq::context_t context;
    zmq::socket_t socket{context, zmq::socket_type::router};
    zmq::poller_t<> poller;
    CHECK_NOTHROW(poller.add(socket, ZMQ_POLLIN, nullptr));
    std::vector<zmq::socket_t> sockets;
    sockets.emplace_back(std::move(socket));
    CHECK_THROWS_AS(poller.remove(socket), zmq::error_t);
    CHECK_NOTHROW(poller.remove(sockets[0]));
}

TEST_CASE("poller modify empty throws", "[poller]")
{
    zmq::context_t context;
    zmq::socket_t socket{context, zmq::socket_type::push};
    zmq::poller_t<> poller;
    CHECK_THROWS_AS(poller.modify(socket, ZMQ_POLLIN), zmq::error_t);
}

TEST_CASE("poller modify invalid socket throws", "[poller]")
{
    zmq::context_t context;
    zmq::socket_t a{context, zmq::socket_type::push};
    zmq::socket_t b{std::move(a)};
    zmq::poller_t<> poller;
    CHECK_THROWS_AS(poller.modify(a, ZMQ_POLLIN), zmq::error_t);
}

TEST_CASE("poller modify not added throws", "[poller]")
{
    zmq::context_t context;
    zmq::socket_t a{context, zmq::socket_type::push};
    zmq::socket_t b{context, zmq::socket_type::push};
    zmq::poller_t<> poller;
    CHECK_NOTHROW(poller.add(a, ZMQ_POLLIN, nullptr));
    CHECK_THROWS_AS(poller.modify(b, ZMQ_POLLIN), zmq::error_t);
}

TEST_CASE("poller modify simple", "[poller]")
{
    zmq::context_t context;
    zmq::socket_t a{context, zmq::socket_type::push};
    zmq::poller_t<> poller;
    CHECK_NOTHROW(poller.add(a, ZMQ_POLLIN, nullptr));
    CHECK_NOTHROW(poller.modify(a, ZMQ_POLLIN | ZMQ_POLLOUT));
}

TEST_CASE("poller poll client server", "[poller]")
{
    // Setup server and client
    common_server_client_setup s;

    // Setup poller
    zmq::poller_t<> poller;
    CHECK_NOTHROW(poller.add(s.server, ZMQ_POLLIN, s.server));

    // client sends message
    CHECK_NOTHROW(s.client.send(zmq::message_t{"Hi"}));

    // wait for message and verify events
    std::vector<zmq_poller_event_t> events(1);
    CHECK(1 == poller.wait_all(events, std::chrono::milliseconds{500}));
    CHECK(ZMQ_POLLIN == events[0].events);

    // Modify server socket with pollout flag
    CHECK_NOTHROW(poller.modify(s.server, ZMQ_POLLIN | ZMQ_POLLOUT));
    CHECK(1 == poller.wait_all(events, std::chrono::milliseconds{500}));
    CHECK((ZMQ_POLLIN | ZMQ_POLLOUT) == events[0].events);
}

TEST_CASE("poller wait one return", "[poller]")
{
    // Setup server and client
    common_server_client_setup s;

    // Setup poller
    zmq::poller_t<> poller;
    CHECK_NOTHROW(poller.add(s.server, ZMQ_POLLIN, nullptr));

    // client sends message
    CHECK_NOTHROW(s.client.send(zmq::message_t{"Hi"}));

    // wait for message and verify events
    std::vector<zmq_poller_event_t> events(1);
    CHECK(1 == poller.wait_all(events, std::chrono::milliseconds{500}));
}

TEST_CASE("poller wait on move constructed", "[poller]")
{
    common_server_client_setup s;
    CHECK_NOTHROW(s.client.send(zmq::message_t{"Hi"}));
    zmq::poller_t<> a;
    CHECK_NOTHROW(a.add(s.server, ZMQ_POLLIN, nullptr));
    zmq::poller_t<> b{std::move(a)};
    std::vector<zmq_poller_event_t> events(1);
    /// \todo the actual error code should be checked
    CHECK_THROWS_AS(a.wait_all(events, std::chrono::milliseconds{10}), zmq::error_t);
    CHECK(1 == b.wait_all(events, std::chrono::milliseconds{-1}));
}

TEST_CASE("poller wait on move assigned", "[poller]")
{
    common_server_client_setup s;
    CHECK_NOTHROW(s.client.send(zmq::message_t{"Hi"}));
    zmq::poller_t<> a;
    CHECK_NOTHROW(a.add(s.server, ZMQ_POLLIN, nullptr));
    zmq::poller_t<> b;
    b = {std::move(a)};
    /// \todo the TEST_CASE error code should be checked
    std::vector<zmq_poller_event_t> events(1);
    CHECK_THROWS_AS(a.wait_all(events, std::chrono::milliseconds{10}), zmq::error_t);
    CHECK(1 == b.wait_all(events, std::chrono::milliseconds{-1}));
}

TEST_CASE("poller remove from handler", "[poller]")
{
    constexpr auto ITER_NO = 10;

    // Setup servers and clients
    std::vector<common_server_client_setup> setup_list;
    for (auto i = 0; i < ITER_NO; ++i)
        setup_list.emplace_back(common_server_client_setup{});

    // Setup poller
    zmq::poller_t<> poller;
    for (auto i = 0; i < ITER_NO; ++i) {
        CHECK_NOTHROW(poller.add(setup_list[i].server, ZMQ_POLLIN, nullptr));
    }
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
    std::vector<zmq_poller_event_t> events(ITER_NO);
    CHECK(ITER_NO == poller.wait_all(events, std::chrono::milliseconds{-1}));
}

#endif
