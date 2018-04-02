#include <gtest/gtest.h>
#include <zmq.hpp>

TEST(context, create_default_destroy)
{
    zmq::context_t context;
}

TEST(context, create_close)
{
    zmq::context_t context;
    context.close();
    
    ASSERT_EQ(NULL, (void*)context);
}

TEST(socket, create_destroy)
{
    zmq::context_t context;
    zmq::socket_t socket(context, ZMQ_ROUTER);
}
