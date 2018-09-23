#include "testutil.hpp"

#ifdef ZMQ_CPP11
#include <thread>
#include <mutex>
#include <condition_variable>

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
TEST_CASE("monitor init event count", "[monitor]")
{
    common_server_client_setup s{false};
    mock_monitor_t monitor;

    const int expected_event_count = 2;
    int event_count = 0;
    auto count_event = [&event_count](const zmq_event_t &, const char *) {
        ++event_count;
    };

    monitor.init(s.client, "inproc://foo");

    CHECK_FALSE(monitor.check_event(0));
    s.init();

    while (monitor.check_event(100) && monitor.total < expected_event_count) {
    }
    CHECK(monitor.connect_delayed == 1);
    CHECK(monitor.connected == 1);
}

TEST_CASE("monitor init abort", "[monitor]")
{
    class mock_monitor : public mock_monitor_t
    {
      public:
        mock_monitor(std::function<void(void)> handle_connected)
            : handle_connected{std::move(handle_connected)}
        {}

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

    mock_monitor monitor([&]() {
        std::lock_guard<std::mutex> lock(mutex);
        done = true;
        cond_var.notify_one();
    });
    monitor.init(s.client, "inproc://foo");

    auto thread = std::thread([&monitor] {
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
#endif
