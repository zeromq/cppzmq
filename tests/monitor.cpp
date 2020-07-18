#include "testutil.hpp"

#ifdef ZMQ_CPP11
#include <thread>
#include <mutex>
#include <condition_variable>
#include <functional>

class mock_monitor_t : public zmq::monitor_t
{
public:
    void on_event_connect_delayed(const zmq_event_t &, const char *) ZMQ_OVERRIDE
    {
        ++connect_delayed;
        ++total;
    }

    void on_event_connected(const zmq_event_t &, const char *) ZMQ_OVERRIDE
    {
        ++connected;
        ++total;
    }

    int total{0};
    int connect_delayed{0};
    int connected{0};
};

#endif

TEST_CASE("monitor create destroy", "[monitor]")
{
    zmq::monitor_t monitor;
}

#if defined(ZMQ_CPP11)
TEST_CASE("monitor move construct", "[monitor]")
{
    zmq::context_t ctx;
    zmq::socket_t sock(ctx, ZMQ_DEALER);
    SECTION("move ctor empty") {
        zmq::monitor_t monitor1;
        zmq::monitor_t monitor2 = std::move(monitor1);
    }
    SECTION("move ctor init") {
        zmq::monitor_t monitor1;
        monitor1.init(sock, "inproc://monitor-client");
        zmq::monitor_t monitor2 = std::move(monitor1);
    }
}

TEST_CASE("monitor move assign", "[monitor]")
{
    zmq::context_t ctx;
    zmq::socket_t sock(ctx, ZMQ_DEALER);
    SECTION("move assign empty") {
        zmq::monitor_t monitor1;
        zmq::monitor_t monitor2;
        monitor1 = std::move(monitor2);
    }
    SECTION("move assign init") {
        zmq::monitor_t monitor1;
        monitor1.init(sock, "inproc://monitor-client");
        zmq::monitor_t monitor2;
        monitor2 = std::move(monitor1);
    }
    SECTION("move assign init both") {
        zmq::monitor_t monitor1;
        monitor1.init(sock, "inproc://monitor-client");
        zmq::monitor_t monitor2;
        zmq::socket_t sock2(ctx, ZMQ_DEALER);
        monitor2.init(sock2, "inproc://monitor-client2");
        monitor2 = std::move(monitor1);
    }
}

TEST_CASE("monitor init event count", "[monitor]")
{
    common_server_client_setup s{false};
    mock_monitor_t monitor;

    const int expected_event_count = 2;
    monitor.init(s.client, "inproc://foo");

    CHECK_FALSE(monitor.check_event(0));
    s.init();

    while (monitor.check_event(100) && monitor.total < expected_event_count) {
    }
    CHECK(monitor.connect_delayed == 1);
    CHECK(monitor.connected == 1);
    CHECK(monitor.total == expected_event_count);
}

TEST_CASE("monitor init abort", "[monitor]")
{
    class mock_monitor : public mock_monitor_t
    {
    public:
        mock_monitor(std::function<void(void)> handle_connected) :
            handle_connected{std::move(handle_connected)}
        {
        }

        void on_event_connected(const zmq_event_t &e, const char *m) ZMQ_OVERRIDE
        {
            mock_monitor_t::on_event_connected(e, m);
            handle_connected();
        }

        std::function<void(void)> handle_connected;
    };

    common_server_client_setup s(false);

    std::mutex mutex;
    std::condition_variable cond_var;
    bool done{false};

    mock_monitor monitor([&]()
    {
        std::lock_guard<std::mutex> lock(mutex);
        done = true;
        cond_var.notify_one();
    });
    monitor.init(s.client, "inproc://foo");

    auto thread = std::thread([&monitor]
    {
        while (monitor.check_event(-1)) {
        }
    });

    s.init();
    {
        std::unique_lock<std::mutex> lock(mutex);
        CHECK(cond_var.wait_for(lock, std::chrono::seconds(1),
            [&done] { return done; }));
    }
    CHECK(monitor.connect_delayed == 1);
    CHECK(monitor.connected == 1);
    monitor.abort();
    thread.join();
}


TEST_CASE("monitor from move assigned socket", "[monitor]")
{
    zmq::context_t ctx;
    zmq::socket_t sock;
    sock = std::move([&ctx] {
        zmq::socket_t sock(ctx, ZMQ_DEALER);
        return sock;
    }());
    zmq::monitor_t monitor1;
    monitor1.init(sock, "inproc://monitor-client");
    // On failure, this test might hang indefinitely instead of immediately
    // failing
}
#endif
