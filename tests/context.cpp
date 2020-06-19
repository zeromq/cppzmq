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
    CHECK(NULL == context.handle());
}

TEST_CASE("context shutdown", "[context]")
{
    zmq::context_t context;
    context.shutdown();
    CHECK(NULL != context.handle());
    context.close();
    CHECK(NULL == context.handle());
}

TEST_CASE("context shutdown again", "[context]")
{
    zmq::context_t context;
    context.shutdown();
    context.shutdown();
    CHECK(NULL != context.handle());
    context.close();
    CHECK(NULL == context.handle());
}

#ifdef ZMQ_CPP11
TEST_CASE("context swap", "[context]")
{
    zmq::context_t context1;
    zmq::context_t context2;
    using std::swap;
    swap(context1, context2);
}

TEST_CASE("context - use socket after shutdown", "[context]")
{
    zmq::context_t context;
    zmq::socket_t sock(context, zmq::socket_type::rep);
    context.shutdown();
    try
    {
        sock.connect("inproc://test");
        zmq::message_t msg;
        (void)sock.recv(msg, zmq::recv_flags::dontwait);
        REQUIRE(false);
    }
    catch (const zmq::error_t& e)
    {
        REQUIRE(e.num() == ETERM);
    }
}

TEST_CASE("context set/get options", "[context]")
{
    zmq::context_t context;
#if defined(ZMQ_BLOCKY) && defined(ZMQ_IO_THREADS)
    context.set(zmq::ctxopt::blocky, false);
    context.set(zmq::ctxopt::io_threads, 5);
    CHECK(context.get(zmq::ctxopt::io_threads) == 5);
#endif

    CHECK_THROWS_AS(
        context.set(static_cast<zmq::ctxopt>(-42), 5),
        const zmq::error_t &);

    CHECK_THROWS_AS(
        context.get(static_cast<zmq::ctxopt>(-42)),
        const zmq::error_t &);
}
#endif
