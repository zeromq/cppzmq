#include "testutil.hpp"

#include <gmock/gmock.h>
#ifdef ZMQ_CPP11
#include <thread>
#include <mutex>
#include <condition_variable>
#endif

class mock_monitor_t : public zmq::monitor_t
{
  public:
    MOCK_METHOD2(on_event_connect_delayed, void(const zmq_event_t &, const char *));
    MOCK_METHOD2(on_event_connected, void(const zmq_event_t &, const char *));
};

TEST(monitor, create_destroy)
{
    zmq::monitor_t monitor;
}

#if defined(ZMQ_CPP11)
TEST(monitor, init_check)
{
    common_server_client_setup s{false};
    mock_monitor_t monitor;

    const int expected_event_count = 2;
    int event_count = 0;
    auto count_event = [&event_count](const zmq_event_t &, const char *) {
        ++event_count;
    };

    EXPECT_CALL(monitor, on_event_connect_delayed(testing::_, testing::_))
      .Times(1)
      .WillOnce(testing::Invoke(count_event));
    EXPECT_CALL(monitor, on_event_connected(testing::_, testing::_))
      .Times(1)
      .WillOnce(testing::Invoke(count_event));

    monitor.init(s.client, "inproc://foo");

    ASSERT_FALSE(monitor.check_event(0));
    s.init();

    while (monitor.check_event(100) && event_count < expected_event_count) {
    }
}

TEST(monitor, init_abort)
{
    common_server_client_setup s(false);
    mock_monitor_t monitor;
    monitor.init(s.client, "inproc://foo");

    std::mutex mutex;
    std::condition_variable cond_var;
    bool done{false};

    EXPECT_CALL(monitor, on_event_connect_delayed(testing::_, testing::_))
      .Times(1);
    EXPECT_CALL(monitor, on_event_connected(testing::_, testing::_))
      .Times(1)
      .WillOnce(testing::Invoke([&](const zmq_event_t &, const char *) {
        std::lock_guard<std::mutex> lock(mutex);
        done = true;
        cond_var.notify_one();
        }));

    auto thread = std::thread([&monitor] {
        while (monitor.check_event(-1)) {
        }
    });

    s.init();
    {
        std::unique_lock<std::mutex> lock(mutex);
        EXPECT_TRUE(cond_var.wait_for(lock, std::chrono::seconds(1),
            [&done] { return done; }));
    }

    monitor.abort();
    thread.join();
}
#endif
