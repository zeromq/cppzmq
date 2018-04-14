#include <gtest/gtest.h>
#include <zmq.hpp>

TEST (message, create_destroy)
{
    zmq::message_t message;
}

TEST (message, constructors)
{
    const std::string hi {"Hi"};
    zmq::message_t hi_msg_a {hi.begin (), hi.end ()};
    ASSERT_EQ (hi_msg_a.size (), hi.size ());
    zmq::message_t hi_msg_b {hi.data (), hi.size ()};
    ASSERT_EQ (hi_msg_b.size (), hi.size ());
    ASSERT_EQ (hi_msg_a, hi_msg_b);
#if defined(ZMQ_BUILD_DRAFT_API) && defined(ZMQ_CPP11)
    zmq::message_t hello_msg_a {"Hello"};
    ASSERT_NE (hi_msg_a, hello_msg_a);
    ASSERT_NE (hi_msg_b, hello_msg_a);
    zmq::message_t hi_msg_c {hi};
    ASSERT_EQ (hi_msg_c, hi_msg_a);
    ASSERT_EQ (hi_msg_c, hi_msg_b);
    ASSERT_NE (hi_msg_c, hello_msg_a);
#endif
#ifdef ZMQ_HAS_RVALUE_REFS
    zmq::message_t hello_msg_b{zmq::message_t{"Hello"}};
    ASSERT_EQ (hello_msg_a, hello_msg_b);
#endif
}
