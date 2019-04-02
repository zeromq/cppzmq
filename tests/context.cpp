#include <catch.hpp>
#include <zmq.hpp>

#if (__cplusplus >= 201703L)
static_assert(std::is_nothrow_swappable<zmq::context_t>::value,
              "context_t should be nothrow swappable");
#endif

TEST_CASE("context construct default and destroy", "[context]")
{
    zmq::context_t context;
}

TEST_CASE("context create, close and destroy", "[context]")
{
    zmq::context_t context;
    context.close();
    CHECK(NULL == (void *) context);
}

#ifdef ZMQ_CPP11
TEST_CASE("context swap", "[context]")
{
    zmq::context_t context1;
    zmq::context_t context2;
    using std::swap;
    swap(context1, context2);
}
#endif
