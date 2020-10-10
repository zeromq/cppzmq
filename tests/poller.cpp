#include "testutil.hpp"

#if defined(ZMQ_BUILD_DRAFT_API) && defined(ZMQ_CPP11) && defined(ZMQ_HAVE_POLLER)

#include <array>
#include <memory>

#ifdef ZMQ_CPP17
static_assert(std::is_nothrow_swappable_v<zmq::poller_t<>>);
#endif
static_assert(sizeof(zmq_poller_event_t) == sizeof(zmq::poller_event<>), "");
static_assert(sizeof(zmq_poller_event_t) == sizeof(zmq::poller_event<zmq::
                  no_user_data>), "");
static_assert(sizeof(zmq_poller_event_t) == sizeof(zmq::poller_event<int>), "");
static_assert(alignof(zmq_poller_event_t) == alignof(zmq::poller_event<>), "");
static_assert(alignof(zmq_poller_event_t) == alignof(zmq::poller_event<int>), "");

static_assert(!std::is_copy_constructible<zmq::poller_t<>>::value,
    "poller_t should not be copy-constructible");
static_assert(!std::is_copy_assignable<zmq::poller_t<>>::value,
    "poller_t should not be copy-assignable");

TEST_CASE("event flags", "[poller]")
{
    CHECK((zmq::event_flags::pollin | zmq::event_flags::pollout)
        == static_cast<zmq::event_flags>(ZMQ_POLLIN | ZMQ_POLLOUT));
    CHECK((zmq::event_flags::pollin & zmq::event_flags::pollout)
        == static_cast<zmq::event_flags>(ZMQ_POLLIN & ZMQ_POLLOUT));
    CHECK((zmq::event_flags::pollin ^ zmq::event_flags::pollout)
        == static_cast<zmq::event_flags>(ZMQ_POLLIN ^ ZMQ_POLLOUT));
    CHECK(~zmq::event_flags::pollin == static_cast<zmq::event_flags>(~ZMQ_POLLIN));
}

TEST_CASE("poller create destroy", "[poller]")
{
    zmq::poller_t<> a;
#ifdef ZMQ_CPP17 // CTAD
    zmq::poller_t b;
    zmq::poller_event e;
#endif
}

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

TEST_CASE("poller swap", "[poller]")
{
    zmq::poller_t<> a;
    zmq::poller_t<> b;
    using std::swap;
    swap(a, b);
}

TEST_CASE("poller move construct non empty", "[poller]")
{
    zmq::context_t context;
    zmq::socket_t socket{context, zmq::socket_type::router};

    zmq::poller_t<> a;
    a.add(socket, zmq::event_flags::pollin);
    zmq::poller_t<> b = std::move(a);
}

TEST_CASE("poller move assign non empty", "[poller]")
{
    zmq::context_t context;
    zmq::socket_t socket{context, zmq::socket_type::router};

    zmq::poller_t<> a;
    a.add(socket, zmq::event_flags::pollin);
    zmq::poller_t<> b;
    b = std::move(a);
}

TEST_CASE("poller add nullptr", "[poller]")
{
    zmq::context_t context;
    zmq::socket_t socket{context, zmq::socket_type::router};
    zmq::poller_t<void> poller;
    CHECK_NOTHROW(poller.add(socket, zmq::event_flags::pollin, nullptr));
}

TEST_CASE("poller add non nullptr", "[poller]")
{
    zmq::context_t context;
    zmq::socket_t socket{context, zmq::socket_type::router};
    zmq::poller_t<int> poller;
    int i;
    CHECK_NOTHROW(poller.add(socket, zmq::event_flags::pollin, &i));
}

#if ZMQ_VERSION >= ZMQ_MAKE_VERSION(4, 3, 0)
// this behaviour was added by https://github.com/zeromq/libzmq/pull/3100
TEST_CASE("poller add handler invalid events type", "[poller]")
{
    zmq::context_t context;
    zmq::socket_t socket{context, zmq::socket_type::router};
    zmq::poller_t<> poller;
    short invalid_events_type = 2 << 10;
    CHECK_THROWS_AS(
        poller.add(socket, static_cast<zmq::event_flags>(invalid_events_type)),
        const zmq::error_t&);
}
#endif

TEST_CASE("poller add handler twice throws", "[poller]")
{
    zmq::context_t context;
    zmq::socket_t socket{context, zmq::socket_type::router};
    zmq::poller_t<> poller;
    poller.add(socket, zmq::event_flags::pollin);
    /// \todo the actual error code should be checked
    CHECK_THROWS_AS(poller.add(socket, zmq::event_flags::pollin),
                    const zmq::error_t&);
}

TEST_CASE("poller wait with no handlers throws", "[poller]")
{
    zmq::poller_t<> poller;
    std::vector<zmq::poller_event<>> events;
    /// \todo the actual error code should be checked
    CHECK_THROWS_AS(poller.wait_all(events, std::chrono::milliseconds{10}),
                    const zmq::error_t&);
}

#if ZMQ_VERSION >= ZMQ_MAKE_VERSION(4, 3, 3)
TEST_CASE("poller add/remove size checks", "[poller]")
{
    zmq::context_t context;
    zmq::socket_t socket{context, zmq::socket_type::router};
    zmq::poller_t<> poller;
    CHECK(poller.size() == 0);
    poller.add(socket, zmq::event_flags::pollin);
    CHECK(poller.size() == 1);
    CHECK_NOTHROW(poller.remove(socket));
    CHECK(poller.size() == 0);
}
#endif

TEST_CASE("poller remove unregistered throws", "[poller]")
{
    zmq::context_t context;
    zmq::socket_t socket{context, zmq::socket_type::router};
    zmq::poller_t<> poller;
    /// \todo the actual error code should be checked
    CHECK_THROWS_AS(poller.remove(socket), const zmq::error_t&);
}

TEST_CASE("poller remove registered empty", "[poller]")
{
    zmq::context_t context;
    zmq::socket_t socket{context, zmq::socket_type::router};
    zmq::poller_t<> poller;
    poller.add(socket, zmq::event_flags::pollin);
    CHECK_NOTHROW(poller.remove(socket));
}

TEST_CASE("poller remove registered non empty", "[poller]")
{
    zmq::context_t context;
    zmq::socket_t socket{context, zmq::socket_type::router};
    zmq::poller_t<int> poller;
    int empty{};
    poller.add(socket, zmq::event_flags::pollin, &empty);
    CHECK_NOTHROW(poller.remove(socket));
}

const std::string hi_str = "Hi";

TEST_CASE("poller poll basic", "[poller]")
{
    common_server_client_setup s;

    CHECK_NOTHROW(s.client.send(zmq::message_t{hi_str}, zmq::send_flags::none));

    zmq::poller_t<int> poller;
    std::vector<zmq::poller_event<int>> events{1};
    int i = 0;
    CHECK_NOTHROW(poller.add(s.server, zmq::event_flags::pollin, &i));
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
    CHECK_THROWS_AS(poller.add(a, zmq::event_flags::pollin), const zmq::error_t&);
}

TEST_CASE("poller remove invalid socket throws", "[poller]")
{
    zmq::context_t context;
    zmq::socket_t socket{context, zmq::socket_type::router};
    zmq::poller_t<> poller;
    CHECK_NOTHROW(poller.add(socket, zmq::event_flags::pollin));
    std::vector<zmq::socket_t> sockets;
    sockets.emplace_back(std::move(socket));
    CHECK_THROWS_AS(poller.remove(socket), const zmq::error_t&);
    CHECK_NOTHROW(poller.remove(sockets[0]));
}

TEST_CASE("poller modify empty throws", "[poller]")
{
    zmq::context_t context;
    zmq::socket_t socket{context, zmq::socket_type::push};
    zmq::poller_t<> poller;
    CHECK_THROWS_AS(poller.modify(socket, zmq::event_flags::pollin),
                    const zmq::error_t&);
}

TEST_CASE("poller modify invalid socket throws", "[poller]")
{
    zmq::context_t context;
    zmq::socket_t a{context, zmq::socket_type::push};
    zmq::socket_t b{std::move(a)};
    zmq::poller_t<> poller;
    CHECK_THROWS_AS(poller.modify(a, zmq::event_flags::pollin), const zmq::error_t&);
}

TEST_CASE("poller modify not added throws", "[poller]")
{
    zmq::context_t context;
    zmq::socket_t a{context, zmq::socket_type::push};
    zmq::socket_t b{context, zmq::socket_type::push};
    zmq::poller_t<> poller;
    CHECK_NOTHROW(poller.add(a, zmq::event_flags::pollin));
    CHECK_THROWS_AS(poller.modify(b, zmq::event_flags::pollin), const zmq::error_t&);
}

TEST_CASE("poller modify simple", "[poller]")
{
    zmq::context_t context;
    zmq::socket_t a{context, zmq::socket_type::push};
    zmq::poller_t<> poller;
    CHECK_NOTHROW(poller.add(a, zmq::event_flags::pollin));
    CHECK_NOTHROW(
        poller.modify(a, zmq::event_flags::pollin | zmq::event_flags::pollout));
}

TEST_CASE("poller poll client server", "[poller]")
{
    // Setup server and client
    common_server_client_setup s;

    // Setup poller
    zmq::poller_t<zmq::socket_t> poller;
    CHECK_NOTHROW(poller.add(s.server, zmq::event_flags::pollin, &s.server));

    // client sends message
    CHECK_NOTHROW(s.client.send(zmq::message_t{hi_str}, zmq::send_flags::none));

    // wait for message and verify events
    std::vector<zmq::poller_event<zmq::socket_t>> events(1);
    CHECK(1 == poller.wait_all(events, std::chrono::milliseconds{500}));
    CHECK(zmq::event_flags::pollin == events[0].events);

    // Modify server socket with pollout flag
    CHECK_NOTHROW(
        poller.modify(s.server, zmq::event_flags::pollin | zmq::event_flags::pollout
        ));
    CHECK(1 == poller.wait_all(events, std::chrono::milliseconds{500}));
    CHECK((zmq::event_flags::pollin | zmq::event_flags::pollout) == events[0].events)
    ;
}

TEST_CASE("poller wait one return", "[poller]")
{
    // Setup server and client
    common_server_client_setup s;

    // Setup poller
    zmq::poller_t<> poller;
    CHECK_NOTHROW(poller.add(s.server, zmq::event_flags::pollin));

    // client sends message
    CHECK_NOTHROW(s.client.send(zmq::message_t{hi_str}, zmq::send_flags::none));

    // wait for message and verify events
    std::vector<zmq::poller_event<>> events(1);
    CHECK(1 == poller.wait_all(events, std::chrono::milliseconds{500}));
}

TEST_CASE("poller wait on move constructed", "[poller]")
{
    common_server_client_setup s;
    CHECK_NOTHROW(s.client.send(zmq::message_t{hi_str}, zmq::send_flags::none));
    zmq::poller_t<> a;
    CHECK_NOTHROW(a.add(s.server, zmq::event_flags::pollin));
    zmq::poller_t<> b{std::move(a)};
    std::vector<zmq::poller_event<>> events(1);
    /// \todo the actual error code should be checked
    CHECK_THROWS_AS(a.wait_all(events, std::chrono::milliseconds{10}),
                    const zmq::error_t&);
    CHECK(1 == b.wait_all(events, std::chrono::milliseconds{-1}));
}

TEST_CASE("poller wait on move assigned", "[poller]")
{
    common_server_client_setup s;
    CHECK_NOTHROW(s.client.send(zmq::message_t{hi_str}, zmq::send_flags::none));
    zmq::poller_t<> a;
    CHECK_NOTHROW(a.add(s.server, zmq::event_flags::pollin));
    zmq::poller_t<> b;
    b = {std::move(a)};
    /// \todo the TEST_CASE error code should be checked
    std::vector<zmq::poller_event<>> events(1);
    CHECK_THROWS_AS(a.wait_all(events, std::chrono::milliseconds{10}),
                    const zmq::error_t&);
    CHECK(1 == b.wait_all(events, std::chrono::milliseconds{-1}));
}

TEST_CASE("poller remove from handler", "[poller]")
{
    constexpr size_t ITER_NO = 10;

    // Setup servers and clients
    std::vector<common_server_client_setup> setup_list;
    for (size_t i = 0; i < ITER_NO; ++i)
        setup_list.emplace_back(common_server_client_setup{});

    // Setup poller
    zmq::poller_t<> poller;
    for (size_t i = 0; i < ITER_NO; ++i) {
        CHECK_NOTHROW(poller.add(setup_list[i].server, zmq::event_flags::pollin));
    }
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
    std::vector<zmq::poller_event<>> events(ITER_NO);
    CHECK(ITER_NO == poller.wait_all(events, std::chrono::milliseconds{-1}));
}

#endif
