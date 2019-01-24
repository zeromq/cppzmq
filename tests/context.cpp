#ifdef USE_EXTERNAL_CATCH2
#include <catch2/catch.hpp>
#else
#include <catch.hpp>
#endif
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
