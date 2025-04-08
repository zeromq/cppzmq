#pragma once

#include "zmq_addon.hpp"
#include <boost/asio/any_io_executor.hpp>
#include <boost/system/detail/error_code.hpp>
#include <stdexcept>
#include <utility>
#include <zmq.hpp>


#if !defined(ZMQ_CPP20) && defined(CPPZMQ_ENABLE_CORRAL_COROUTINE)
#error "Coroutine support enabled with a C++ standard lower than C++20"
#endif
#if defined(CPPZMQ_ENABLE_CORRAL_COROUTINE)
#include <corral/Task.h>
#include <boost/asio/posix/stream_descriptor.hpp>
#include <boost/asio/windows/stream_handle.hpp>
#endif


#ifdef ZMQ_CPP20
// As coroutine support is only avaiable for C++20 and upward,
// there is no point in using the ZMQ_XXX macros for compatibility.
// Everything that's available in C++20 can be used here (e.g. inline, noexcept).
namespace zmq
{

namespace async::corral
{

using native_fd_watcher_t =
#if defined(_WIN32) || defined(_WIN64)
  boost::asio::windows::stream_handle;
#else
  boost::asio::posix::stream_descriptor;
#endif

class recv_error_t : public std::exception
{
  public:
    explicit recv_error_t() noexcept = default;
    virtual const char *what() const noexcept override
    {
        return "Failed receiving ZeroMQ message.";
    }
};

class send_error_t : public std::exception
{
  public:
    explicit send_error_t() noexcept = default;
    virtual const char *what() const noexcept override
    {
        return "Failed sending ZeroMQ message.";
    }
};

namespace details
{

template<std::invocable<> Fn> struct [[nodiscard]] defer_guard
{
    [[no_unique_address, msvc::no_unique_address]] Fn fn;

    constexpr defer_guard(Fn &&f) noexcept(
      ::std::is_nothrow_move_constructible_v<Fn>) :
        fn(::std::move(f))
    {
    }

    constexpr ~defer_guard() noexcept(::std::is_nothrow_invocable_v<Fn>) { fn(); }
};

///
/// \brief Asynchronously send message to a ZeroMQ socket.
/// \return [zmq::message_t] - The message to be sent.
/// \throw [send_error_t] - On send error.
///
struct [[nodiscard]] async_send_awaitable_t
{
  public /* ctor */:
    inline async_send_awaitable_t(native_fd_watcher_t &watcher,
                                  zmq::socket_ref socket_ref,
                                  zmq::message_t message,
                                  zmq::send_flags flags,
                                  bool &is_sending_multipart) :
        m_wait(watcher),
        m_socket(socket_ref),
        m_msg(std::move(message)),
        m_flags(flags),
        m_is_sending_multipart(is_sending_multipart)
    {
    }

  public /* interface */:
    inline auto enable_flag(zmq::send_flags flag) { m_flags = m_flags | flag; }
    inline auto disable_flag(zmq::send_flags flag) { m_flags = m_flags & (~flag); }

  public /* awaitable */:
    ///
    /// \brief An optimization which avoids suspending the coroutine if it isn't necessary.
    /// \return true - when there are already poll-out events and `await_resume` will be
    /// immediately invoked.
    ///
    inline bool await_ready() const noexcept
    {
        return !m_is_sending_multipart && has_pollout_events();
    }

    inline auto await_resume()
    {
        auto result =
          m_socket.send(std::move(m_msg), zmq::send_flags::dontwait | m_flags);
        if (!result) [[unlikely]]
            throw send_error_t{};
    }

    inline void await_suspend(std::coroutine_handle<> h) noexcept
    {
        m_wait.async_wait(native_fd_watcher_t::wait_write,
                          [this, h](boost::system::error_code err) -> void {
                              if (!err) {
                                  if (!m_is_sending_multipart
                                      && has_pollout_events()) {
                                      if (h)
                                          h.resume();
                                  } else {
                                      // schedule for another awake.
                                      this->await_suspend(h);
                                  }
                              }
                          });
    }

  public /* corral extensions */:
    inline bool await_cancel(std::coroutine_handle<> h) noexcept
    {
        m_wait.cancel();
        return true;
    }

    inline bool await_must_resume() const noexcept { return false; }

  private:
    inline bool has_pollout_events() const noexcept
    {
        return m_socket.get(zmq::sockopt::events) & ZMQ_POLLOUT;
    }

  private:
    native_fd_watcher_t &m_wait;
    zmq::socket_ref m_socket;
    zmq::message_t m_msg;
    zmq::send_flags m_flags;
    bool const &m_is_sending_multipart;
};

///
/// \brief Asynchronously receive message from a ZeroMQ socket.
/// \return [zmq::message_t] - The received message.
/// \throw [recv_error_t] - On receive error.
///
struct [[nodiscard]] async_recv_awaitable_t
{
  public /* ctor */:
    inline async_recv_awaitable_t(native_fd_watcher_t &watcher,
                                  zmq::socket_ref socket_ref,
                                  zmq::recv_flags flags,
                                  bool &is_receiving_multipart) :
        m_wait(watcher),
        m_socket(socket_ref),
        m_flags(flags),
        m_is_receiving_multipart(is_receiving_multipart)
    {
    }

  public /* awaitable */:
    ///
    /// \brief An optimization which avoids suspending the coroutine if it isn't necessary.
    /// \return true - when there are already poll-in events and `await_resume` will be
    /// immediately invoked.
    ///
    inline bool await_ready() const noexcept
    {
        return !m_is_receiving_multipart && has_pollin_events();
    }

    [[nodiscard]] inline auto await_resume() -> zmq::message_t
    {
        zmq::message_t msg{};
        auto result = m_socket.recv(msg, zmq::recv_flags::dontwait | m_flags);
        if (!result) [[unlikely]]
            throw recv_error_t{};
        return msg;
    }

    inline void await_suspend(std::coroutine_handle<> h) noexcept
    {
        m_wait.async_wait(
          native_fd_watcher_t::wait_read,
          [this, h](boost::system::error_code err) -> void {
              if (!err) {
                  // only resume if there are poll-in events
                  if (!m_is_receiving_multipart && has_pollin_events()) {
                      if (h)
                          h.resume();
                  } else {
                      // If the file descriptor can be read but socket doesn't
                      // have poll-in event, schedule for another awake.
                      this->await_suspend(h);
                  }
              }
          });
    }

  public /* corral extensions */:
    inline bool await_cancel(std::coroutine_handle<> h) noexcept
    {
        m_wait.cancel();
        return true;
    }

    [[nodiscard]] inline bool await_must_resume() const noexcept { return false; }

  private:
    [[nodiscard]] inline bool has_pollin_events() const noexcept
    {
        return m_socket.get(zmq::sockopt::events) & ZMQ_POLLIN;
    }

  private:
    native_fd_watcher_t &m_wait;
    zmq::socket_ref m_socket;
    zmq::recv_flags m_flags;
    bool const &m_is_receiving_multipart;
};

///
/// \brief Asynchronously send message to a ZeroMQ socket.
/// \return [zmq::multipart_t] - The message to be sent.
/// \throw [send_error_t] - On send error.
///
struct [[nodiscard]] async_send_multipart_awaitable_t
{
  public /* ctor */:
    inline async_send_multipart_awaitable_t(native_fd_watcher_t &watcher,
                                            zmq::socket_ref socket_ref,
                                            zmq::multipart_t message,
                                            zmq::send_flags flags,
                                            bool &is_sending_multipart) :
        m_wait(watcher),
        m_socket(socket_ref),
        m_message(std::move(message)),
        // `sndmore` flag should not be controlled by the user.
        m_flags(flags & ~(zmq::send_flags::sndmore)),
        m_is_sending_multipart(is_sending_multipart)
    {
    }

  public /* awaitable */:
    ///
    /// \note The early-resume optimization isn't available here:
    /// The filter must be invoked to properly check if it's OK to submit message
    /// into the send queue.
    ///
    inline bool await_ready() const noexcept { return false; }

    inline void await_resume()
    {
        if (!m_succeed) [[unlikely]]
            throw send_error_t{};
    }

    /// \note Refer to resume_filter for resumption behavior
    inline void await_suspend(std::coroutine_handle<> h) noexcept
    {
        m_started = true;
        m_wait.async_wait(native_fd_watcher_t::wait_write,
                          [this, h](boost::system::error_code err) -> void {
                              if (!err) {
                                  this->resume_filter(h, m_message);
                              }
                          });
    }

  public /* corral extensions */:
    inline bool await_cancel(std::coroutine_handle<> h) noexcept
    {
        m_wait.cancel();
        // If either the send proccess hasn't started, or it has finished,
        // Cancellation does not require any additional efforts.
        if (!m_started || m_succeed) [[likely]]
            return true;

        // Otherwise:
        // The multipart message should never be queued half-way without being sent,
        // continue sending the message
        this->await_suspend(h);
        return false;
    }

    inline bool await_must_resume() const noexcept
    {
        return m_started && !m_succeed;
    }

  private:
    inline bool has_pollout_events() const noexcept
    {
        return m_socket.get(zmq::sockopt::events) & ZMQ_POLLOUT;
    }

    ///
    /// \brief This function is used as a callback. It's called when the file descriptor
    /// can be written with data.
    ///
    inline void resume_filter(std::coroutine_handle<> h,
                              zmq::multipart_t &msg) noexcept
    {
        if (!m_is_sending_multipart && has_pollout_events()) {
            m_is_sending_multipart = true;
            defer_guard defer = [&] { m_is_sending_multipart = false; };

            ///
            /// \brief This boolean is used to indicate if this function had sent anything:
            /// On sending failure, it can either be:
            ///
            /// - that the queue is full (and therefore should be suspended
            ///   and scheduled for another awake).
            /// - or that the socket is disconnected.
            ///
            bool sent_something = false;

            // Instead of repeatedly suspending and awaking, try to fill the send queue
            // once, and fallback to suspension if no message can be submitted to the send
            // queue at the moment.
            while (true) {
                auto m = msg.pop();
                const bool more = !msg.empty();

                const auto result = m_socket.send(
                  std::move(m),
                  (more ? zmq::send_flags::sndmore : zmq::send_flags::none)
                    | zmq::send_flags::dontwait | m_flags);

                if (result) [[likely]] {
                    sent_something = true;
                } else [[unlikely]] {
                    if (sent_something)
                        goto schedule_another_awake;

                    // Resume without setting the success flag,
                    // to indicate that the operation has failed.
                    goto resume_coroutine;
                }

                if (!more) {
                    m_succeed = true;
                    goto resume_coroutine;
                }
            }
        }

    schedule_another_awake:
        /// If the above operations neither success nor fail, it means that
        /// the multipart message haven't been sent. Schedule for another awake.
        this->await_suspend(h);
        return;

    resume_coroutine:
        if (h)
            h.resume();
        return;
    }

  private:
    native_fd_watcher_t &m_wait;
    zmq::socket_ref m_socket;
    zmq::multipart_t m_message;
    bool m_started = false;
    bool m_succeed = false;
    zmq::send_flags m_flags;
    bool &m_is_sending_multipart;
};

///
/// \brief Asynchronously receive message from a ZeroMQ socket.
/// \return [zmq::multipart_t] - The received message.
/// \throw [recv_error_t] - On receive error.
///
struct [[nodiscard]] async_recv_multipart_awaitable_t
{
  public /* ctor */:
    inline async_recv_multipart_awaitable_t(native_fd_watcher_t &watcher,
                                            zmq::socket_ref socket_ref,
                                            zmq::recv_flags flags,
                                            bool &is_receiving_multipart) :
        m_wait(watcher),
        m_socket(socket_ref),
        m_flags(flags),
        m_is_receiving_multipart(is_receiving_multipart)
    {
    }

  public /* awaitable */:
    ///
    /// \brief An optimization which avoids suspending the coroutine if it isn't necessary.
    /// \return true - when there are already poll-in events and `await_resume` will be
    /// immediately invoked.
    ///
    inline bool await_ready() const noexcept
    {
        return !m_is_receiving_multipart && has_pollin_events();
    }

    ///
    /// \note ZeroMQ's multipart message is not equivalent to streams in other libraries.
    /// A multipart message is atomic: either it's fully received or it's not.
    /// It's like sending a std::deque in a `zmq::message_t`.
    /// Therefore it's not necessary to repeatedly suspend the coroutine in the receive loop:
    /// When poll-in event comes in, the whole multipart message has already been received by
    /// ZeroMQ.
    ///
    [[nodiscard]] inline auto await_resume() -> zmq::multipart_t
    {
        if (m_is_receiving_multipart) [[unlikely]]
            throw std::runtime_error{
              "Internal error: This socket has multiple readers"};

        m_is_receiving_multipart = true;
        defer_guard defer = [&] { m_is_receiving_multipart = false; };
        zmq::multipart_t msg;
        while (true) {
            zmq::message_t m;
            auto result = m_socket.recv(m, zmq::recv_flags::dontwait | m_flags);
            if (!result) [[unlikely]]
                throw recv_error_t{};
            if (m.more()) {
                msg.add(std::move(m));
                continue;
            } else {
                msg.add(std::move(m));
                break;
            }
        }
        return msg;
    }

    inline void await_suspend(std::coroutine_handle<> h) noexcept
    {
        m_wait.async_wait(
          native_fd_watcher_t::wait_read,
          [this, h](boost::system::error_code err) -> void {
              if (!err) {
                  if (!m_is_receiving_multipart && has_pollin_events()) {
                      if (h)
                          h.resume();
                  } else {
                      // If the file descriptor can be read but socket doesn't
                      // have poll-in event, schedule for another awake.
                      this->await_suspend(h);
                  }
              }
          });
    }

  public /* corral extensions */:
    inline bool await_cancel(std::coroutine_handle<> h) noexcept
    {
        m_wait.cancel();
        return true;
    }

    [[nodiscard]] inline bool await_must_resume() const noexcept { return false; }

  private:
    inline bool has_pollin_events() const noexcept
    {
        return m_socket.get(zmq::sockopt::events) & ZMQ_POLLIN;
    }

  private:
    native_fd_watcher_t &m_wait;
    zmq::socket_ref m_socket;
    zmq::recv_flags m_flags;
    bool &m_is_receiving_multipart;
};
}


struct socket_t
{
  public /* constructors */:
    inline socket_t(boost::asio::any_io_executor io_executor,
                    ::zmq::context_t &context,
                    ::zmq::socket_type type) :
        m_socket(context, type),
        m_watcher(io_executor, m_socket.get(::zmq::sockopt::fd))
    {
    }

    /// syntax sugar to automatically get executor from io_context
    inline socket_t(boost::asio::io_context &io_context,
                    zmq::context_t &context,
                    zmq::socket_type type) :
        socket_t(io_context.get_executor(), context, type)
    {
    }

    socket_t(socket_t &&) noexcept = default;


    ///
    /// \brief Releases the file descriptor without closing it.
    /// stream_descriptor attempts to close the file descriptor on destruction,
    /// this prevents that from happening because the file descriptor should be
    /// managed by ZeroMQ instead.
    ///
    inline ~socket_t() { m_watcher.release(); }

  public /* interfaces */:
    inline auto send(zmq::message_t msg,
                     zmq::send_flags flags = zmq::send_flags::none)
      -> details::async_send_awaitable_t
    {
        return {m_watcher, m_socket, std::move(msg), flags, m_is_sending_multipart};
    }

    inline auto recv(zmq::recv_flags flags = zmq::recv_flags::none)
      -> details::async_recv_awaitable_t
    {
        return {m_watcher, m_socket, flags, m_is_receiving_multipart};
    }

    inline auto send_multipart(zmq::multipart_t msg,
                               zmq::send_flags flags = zmq::send_flags::none)
      -> details::async_send_multipart_awaitable_t
    {
        return {m_watcher, m_socket, std::move(msg), flags, m_is_sending_multipart};
    }

    inline auto send(zmq::multipart_t msg,
                     zmq::send_flags flags = zmq::send_flags::none)
    {
        return this->send_multipart(std::move(msg), flags);
    }

    inline auto recv_multipart(zmq::recv_flags flags = zmq::recv_flags::none)
      -> details::async_recv_multipart_awaitable_t
    {
        return {m_watcher, m_socket, flags, m_is_receiving_multipart};
    }

  public /* proxy */:
    inline decltype(auto) bind(const char *addr) { return m_socket.bind(addr); }
    inline decltype(auto) bind(std::string addr)
    {
        return m_socket.bind(std::move(addr));
    }
    inline decltype(auto) connect(const char *addr)
    {
        return m_socket.connect(addr);
    }
    inline decltype(auto) connect(std::string addr)
    {
        return m_socket.connect(std::move(addr));
    }
    inline decltype(auto) close() { return m_socket.close(); }

    inline decltype(auto) swap(zmq::socket_t &other) { return m_socket.swap(other); }
    inline decltype(auto) disconnect(const char *addr)
    {
        return m_socket.disconnect(addr);
    }
    inline decltype(auto) disconnect(std::string addr)
    {
        return m_socket.disconnect(std::move(addr));
    }

    template<int Opt, class T, bool BoolUnit>
    inline decltype(auto) get(sockopt::integral_option<Opt, T, BoolUnit> _)
    {
        return m_socket.get<Opt, T, BoolUnit>(_);
    }

    inline decltype(auto) handle() { return m_socket.handle(); }
    inline decltype(auto) join(const char *group) { return m_socket.join(group); }
    inline decltype(auto) leave(const char *group) { return m_socket.leave(group); }

    // Set integral socket option, e.g.
    // `socket.set(zmq::sockopt::linger, 0)`
    template<int Opt, class T, bool BoolUnit>
    void set(sockopt::integral_option<Opt, T, BoolUnit> _, const T &val)
    {
        m_socket.set(_, val);
    }

    // Set integral socket option from boolean, e.g.
    // `socket.set(zmq::sockopt::immediate, false)`
    template<int Opt, class T>
    void set(sockopt::integral_option<Opt, T, true> _, bool val)
    {
        m_socket.set(_, val);
    }

    // Set array socket option, e.g.
    // `socket.set(zmq::sockopt::plain_username, "foo123")`
    template<int Opt, int NullTerm>
    void set(sockopt::array_option<Opt, NullTerm> _, const char *buf)
    {
        m_socket.set(_, buf);
    }

    // Set array socket option, e.g.
    // `socket.set(zmq::sockopt::routing_id, zmq::buffer(id))`
    template<int Opt, int NullTerm>
    void set(sockopt::array_option<Opt, NullTerm> _, const_buffer buf)
    {
        m_socket.set(_, buf);
    }

    // Set array socket option, e.g.
    // `socket.set(zmq::sockopt::routing_id, id_str)`
    template<int Opt, int NullTerm>
    void set(sockopt::array_option<Opt, NullTerm> _, const std::string &buf)
    {
        m_socket.set(_, buf);
    }

    // Set array socket option, e.g.
    // `socket.set(zmq::sockopt::routing_id, id_str)`
    template<int Opt, int NullTerm>
    void set(sockopt::array_option<Opt, NullTerm> _, std::string_view buf)
    {
        m_socket.set(_, buf);
    }

    inline decltype(auto) unbind(const char *addr) { return m_socket.unbind(addr); }
    inline decltype(auto) unbind(std::string addr)
    {
        return m_socket.unbind(std::move(addr));
    }

    auto &get_socket() { return m_socket; }

  private:
    zmq::socket_t m_socket;
    native_fd_watcher_t m_watcher;
    /// \brief true - if this socket is currently processing a multipart message.
    /// This blocks the socket from being requested to process another multipart
    /// message at the same time.
    bool m_is_sending_multipart = false;
    bool m_is_receiving_multipart = false;
};


}

#endif

} // namespace zmq

#undef RESULT