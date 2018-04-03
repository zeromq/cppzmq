#include <gtest/gtest.h>
#include <zmq.hpp>

#if defined(ZMQ_CPP11) && defined(ZMQ_BUILD_DRAFT_API)
TEST(poller, create_destroy)
{
    zmq::poller_t context;
}
#endif
