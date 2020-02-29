#include <catch.hpp>

#include <zmq_actor.hpp>
#ifdef ZMQ_CPP11
#include <future>

#if (__cplusplus >= 201703L)
static_assert(std::is_nothrow_swappable<zmq::socket_t>::value,
              "socket_t should be nothrow swappable");
#endif

static
void myactor(zmq::socket_t& pipe, std::string greeting, bool fast_exit)
{
    pipe.send(zmq::message_t{}, zmq::send_flags::none);
    if (fast_exit) {
        return;
    }
    zmq::message_t rmsg;
    auto res = pipe.recv(rmsg);
}


TEST_CASE("actor external termination", "[actor]")
{
    zmq::context_t context;
    {
        zmq::actor_t actor(context, myactor, "hello world", false);
        actor.pipe().send(zmq::message_t{}, zmq::send_flags::none);
    }

}

TEST_CASE("actor internal termination", "[actor]")
{
    zmq::context_t context;
    {
        zmq::actor_t actor(context, myactor, "fast exit", true);
        actor.pipe().send(zmq::message_t{}, zmq::send_flags::none);
    }
}

#endif  // ZMQ_CPP11
