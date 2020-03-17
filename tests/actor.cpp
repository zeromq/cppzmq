#include <catch.hpp>
#include <zmq_actor.hpp>
#ifdef ZMQ_CPP11
#include <future>



// use to test actor function arg is fully moved
struct MoveNoCopy {
    MoveNoCopy() { }
    MoveNoCopy(MoveNoCopy &&rhs) ZMQ_NOTHROW { }
    MoveNoCopy &operator=(MoveNoCopy &&rhs) ZMQ_NOTHROW { }
    MoveNoCopy(const MoveNoCopy &) ZMQ_DELETED_FUNCTION;
    void operator=(const MoveNoCopy &) ZMQ_DELETED_FUNCTION;
};


static
void czmq_style_actor(zmq::socket_t sock, std::string greeting, bool fast_exit, MoveNoCopy mnc)
{
    // CZMQ zactor_t startup protocol requires actor to send a
    // "signal" signifying "ready".  The app is expected to wait for
    // this message before continuing with its own processing.
    sock.send(zmq::message_t{"",1}, zmq::send_flags::none);

    // If true, actor exits before app tells it to.
    if (fast_exit) {
        return;
    }

    zmq::message_t rmsg;

    // This message represents an application protocol implemented in the test.
    auto res1 = sock.recv(rmsg);
    CHECK(rmsg.size() == 0);

    // The CZMQ zactor_t shutdown protocol has app sending actor a
    // terminate message.
    auto res2 = sock.recv(rmsg);
    CHECK(rmsg.to_string() == "$TERM");
}

static
void noproto_actor(zmq::socket_t sock, std::string greeting, bool fast_exit, MoveNoCopy mnc)
{
    // This is a bare cppzmq actor which implements no
    // startup/shutdown protocol.

    if (fast_exit) {
        return;
    }

    zmq::message_t rmsg;
    // the test protocol is one payload message.
    auto res1 = sock.recv(rmsg);
    CHECK(rmsg.size() == 0);

}

static
void emulate_czmq(bool fast_exit)
{
    zmq::context_t context;
    {
        zmq::actor_t actor(context, czmq_style_actor, "hello world", fast_exit, MoveNoCopy());

        // Recv the CZMQ "actor startup" protocol "ready" message
        // before continuing.  This is done in the "constructor" of
        // CZMQ zactor_t.
        zmq::message_t msg;
        auto res1 = actor.link().recv(msg); 
        CHECK(res1);
        CHECK(msg.size() == 1);

        // When this tests fast_exit=true and the actor function exits
        // first then socket sends will return false.

        // Send the "test protocol" payload message.
        auto res2 = actor.link().send(zmq::message_t{}, zmq::send_flags::dontwait);
        if (!fast_exit) {
            CHECK(res2.has_value() == true);
        }

        // Send the CZMQ "actor shutdown" protocol message.  This is
        // done in the "destructor" of CZMQ zactor_t.
        auto res3 = actor.link().send(zmq::message_t{"$TERM", 5}, zmq::send_flags::dontwait);
        if (!fast_exit) {
            CHECK(res3.has_value() == true);
        }
    }
}
TEST_CASE("actor external termination czmq emulation", "[actor]")
{
    emulate_czmq(false);
}
TEST_CASE("actor internal termination czmq emulation", "[actor]")
{
    emulate_czmq(true);
}


static
void noproto_fastfull(bool fast_exit)
{
    zmq::context_t context;
    {
        zmq::actor_t actor(context, noproto_actor, "hello world", fast_exit, MoveNoCopy());
        auto res = actor.link().send(zmq::message_t{}, zmq::send_flags::dontwait);
        for (int i=0; i<100000; ++i) { ; }
        CHECK(res.has_value() == true);
    }
}

TEST_CASE("actor external termination cppzmq noproto", "[actor]")
{
    noproto_fastfull(false);
}

TEST_CASE("actor internal termination cppzmq noproto", "[actor]")
{
    noproto_fastfull(true);
}


#endif  // ZMQ_CPP11
