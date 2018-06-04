#include <gtest/gtest.h>
#include <gmock/gmock.h>
#include <zmq.hpp>

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

TEST(monitor, init_check)
{
    zmq::context_t ctx;
    zmq::socket_t bind_socket(ctx, ZMQ_DEALER);

    bind_socket.bind("tcp://127.0.0.1:*");
    char endpoint[255];
    size_t endpoint_len = sizeof(endpoint);
    bind_socket.getsockopt(ZMQ_LAST_ENDPOINT, &endpoint, &endpoint_len);

    zmq::socket_t connect_socket(ctx, ZMQ_DEALER);

    mock_monitor_t monitor;
    EXPECT_CALL(monitor, on_event_connect_delayed(testing::_, testing::_))
      .Times(testing::AtLeast(1));
    EXPECT_CALL(monitor, on_event_connected(testing::_, testing::_))
      .Times(testing::AtLeast(1));

    monitor.init(connect_socket, "inproc://foo");

    ASSERT_FALSE(monitor.check_event(0));
    connect_socket.connect(endpoint);

    while (monitor.check_event(100)) {
    }
}
