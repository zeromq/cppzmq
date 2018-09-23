#include <catch.hpp>
#include <zmq.hpp>

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
