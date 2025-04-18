#include <catch2/catch_all.hpp>
#include <zmq.hpp>

#include <type_traits>
#include <thread>
#include <chrono>

#if defined(ZMQ_CPP11) && defined(ZMQ_HAVE_TIMERS)

static_assert(std::is_default_constructible<zmq::timers>::value);
static_assert(!std::is_copy_constructible<zmq::timers>::value);
static_assert(!std::is_copy_assignable<zmq::timers>::value);

TEST_CASE("timers constructor", "[timers]")
{
    zmq::timers timers;
    CHECK(!timers.timeout().has_value());
}

TEST_CASE("timers add/execute", "[timers]")
{
    using namespace std::chrono_literals;
    zmq::timers timers;
    bool handler_ran = false;
    timers.add(4ms, [](auto, void *arg) { *(bool *) arg = true; }, &handler_ran);
    CHECK(timers.timeout().has_value());
    CHECK(!handler_ran);
    std::this_thread::sleep_for(10ms);
    timers.execute();
    CHECK(handler_ran);
}

TEST_CASE("timers add/cancel", "[timers]")
{
    using namespace std::chrono_literals;
    zmq::timers timers;
    bool handler_ran = false;
    auto id =
      timers.add(4ms, [](auto, void *arg) { *(bool *) arg = true; }, &handler_ran);
    CHECK(timers.timeout().has_value());
    CHECK(!handler_ran);
    timers.cancel(id);
    CHECK(!timers.timeout().has_value());
    CHECK(!handler_ran);
}

TEST_CASE("timers set_interval", "[timers]")
{
    using namespace std::chrono_literals;
    zmq::timers timers;
    bool handler_ran = false;
    // Interval of 4 hours should never run like this
    auto id =
      timers.add(4h, [](auto, void *arg) { *(bool *) arg = true; }, &handler_ran);
    CHECK(timers.timeout().has_value());
    CHECK(!handler_ran);
    // Change the interval to 4ms and wait for it to timeout
    timers.set_interval(id, 4ms);
    std::this_thread::sleep_for(10ms);
    timers.execute();
    CHECK(handler_ran);
}

TEST_CASE("timers reset", "[timers]")
{
    using namespace std::chrono_literals;
    zmq::timers timers;
    bool handler_ran = false;
    auto id =
      timers.add(4ms, [](auto, void *arg) { *(bool *) arg = true; }, &handler_ran);
    CHECK(timers.timeout().has_value());
    std::this_thread::sleep_for(10ms);
    // Available to be executed but we reset it
    timers.reset(id);
    CHECK(timers.timeout().has_value());
    CHECK(!handler_ran);

}

#endif // defined(ZMQ_CPP11) && defined(ZMQ_HAVE_TIMERS)
