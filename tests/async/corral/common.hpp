#pragma once

#include <boost/asio/io_context.hpp>
#include <catch2/catch_all.hpp>
#include <catch2/catch_test_macros.hpp>

#include <chrono>
#include <zmq_async.hpp>
#include <corral/corral.h>
#include <boost/asio/steady_timer.hpp>

template<> struct corral::EventLoopTraits<boost::asio::io_context>
{
    using T = boost::asio::io_context;
    /// Returns a value identifying the event loop.
    /// Traits for any sort of wrappers should return ID
    /// of the wrapped event loop.
    static EventLoopID eventLoopID(T &ex) { return EventLoopID{std::addressof(ex)}; }

    /// Runs the event loop.
    static void run(T &ex) { ex.run(); }

    /// Tells the event loop to exit.
    /// run() should return shortly thereafter.
    static void stop(T &ex) { ex.stop(); }
};


struct [[nodiscard]] Timer
{
    explicit Timer(std::chrono::milliseconds sec, boost::asio::io_context &io) :
        timer{io, sec}
    {
    }
    boost::asio::steady_timer timer;

  public /* awaitable */:
    inline auto await_ready() const noexcept
    {
        return timer.expiry() <= std::chrono::steady_clock::now();
    }

    inline auto await_suspend(std::coroutine_handle<> h) noexcept
    {
        timer.async_wait([h](boost::system::error_code e) {
            if (!e)
                h.resume();
        });
    }

    inline auto await_resume() {}

  public /* corral extensions */:
    inline bool await_cancel(std::coroutine_handle<>) noexcept
    {
        return timer.cancel();
    }

    inline std::false_type await_must_resume() const noexcept { return {}; }
};
