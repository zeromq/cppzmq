#pragma once

#if CPPZMQ_ENABLE_STDEXEC

#include <boost/asio/posix/stream_descriptor.hpp>
#include <boost/asio/windows/stream_handle.hpp>
#include <exec/task.hpp>
#include <exec/when_any.hpp>
#include <print>
#include <stdexec/execution.hpp>
#include <zmq_addon.hpp>

// Defining it as a macro as it's repeated many times and it accepts only string literals.
// Note for implementation guidelines: this macro should be un-defined at the end of the file.
// uppercase isn't used as it hurts readibility. the `macro-` prefix is used instead.
#define macro_discarded_sender_diagnose_message                                     \
    "The returned Sender should be `co_await`-ed or `sync_wait`-ed "                \
    "in order to start executing. Discarded Sender does not do anything."

namespace zmq::async::v1
{

/// \note The alias is used to allow trivially migrating to the standard library
/// implementation once they are widely available.
///
/// Currently, most of the implementations depend on primitives already proposed
/// in std::execution (https://wg21.link/p2300) and will be made available starting
/// with C++26, however, part of the implementations still depend on
/// not-yet-standardarized `exec::task<T>` (https://wg21.link/p3552)
/// and `exec::when_any<T>` (`first_successful`).
namespace STDEXEC = stdexec;

struct socket_error_t : std::exception
{
    enum enum_t : uint8_t
    {
        send_error,
        recv_error,
        null_socket,
        socket_unavailable,
    };

    enum_t type;

    constexpr socket_error_t(enum_t type) : type(type) {}

    inline const char *what() const noexcept override
    {
        switch (type) {
            case send_error:
                return "ZeroMQ error: Failed sending message.";
            case recv_error:
                return "ZeroMQ error: Failed receiving message.";
            case null_socket:
                return "ZeroMQ error: Operating on null socket.";
            case socket_unavailable:
                return R"(ZeroMQ error: 
The socket is currently unavailable. Note:

- Both a send and a receive operation can occur concurrently, but it's not allowed
  send multiple messages concurrently, or receive multiple messages concurrently.
- The socket cannot be reused if a previous multipart message operation failed.
)";
            default:
                return "Unknown ZeroMQ error.";
        }
    }
};

using native_fd_watcher_t =
#if defined(_WIN32) || defined(_WIN64)
  boost::asio::windows::stream_handle;
#else
  boost::asio::posix::stream_descriptor;
#endif

namespace details
{

/// \brief A marker type to disable move construction.
struct immovable
{
    immovable() = default;
    immovable(immovable &&) = delete;
};

consteval bool is_fd_supported(zmq::socket_type type)
{
    switch (type) {
        // According to https://github.com/zeromq/libzmq/issues/2941 ,
        // thread-safe sockets do not support taking out file descriptor.
        // FIXME:
        // - The switch cases are currently incomplete.
        // - I don't know how to use poller
        //   (suggested as an alternative in https://github.com/zeromq/libzmq/pull/3484 ).
#if defined(ZMQ_BUILD_DRAFT_API)
        case zmq::socket_type::server:
        case zmq::socket_type::client:
        case zmq::socket_type::radio:
        case zmq::socket_type::dish:
        case zmq::socket_type::scatter:
        case zmq::socket_type::gather:
        case zmq::socket_type::peer:
            return false;
#endif
        default:
            return true;
    }
}

consteval bool is_multipart_supported(zmq::socket_type type)
{
    switch (type) {
        case zmq::socket_type::req:
        case zmq::socket_type::rep:
        case zmq::socket_type::dealer:
        case zmq::socket_type::router:
            return true;

#if defined(ZMQ_BUILD_DRAFT_API)
        case zmq::socket_type::server:
        case zmq::socket_type::client:
        case zmq::socket_type::radio:
        case zmq::socket_type::dish:
        case zmq::socket_type::scatter:
        case zmq::socket_type::gather:
        case zmq::socket_type::peer:
            return false;
#endif

        default:
            // FIXME: I don't use other socket types, therefore I don't know if they
            // have multipart message support.
            //
            // This `std::abort()` will hard fail the socket template instantiation.
            std::abort();
            break;
    }
}

/// \brief The parameter of `zmq_async_message_operation_state`.
struct async_operation_state_config
{
    enum class mode_t
    {
        send,
        receive,
    } mode;
};

/// \brief The ZeroMQ single message send/receive operation state. The following
/// operations are implemented:
///
/// - Single message send
/// - Single message receive
///
/// \note Implementation guidelines: All functions should be `noexcept`, as
/// there isn't a clear error propogation path *between* two asynchronous
/// operations. The error propagation is handled by std::execution
/// implementation instead, using the `set_error` operation.
///
/// \note Whether is operation actually thread safe depends on the socket
/// type being used. The implementation doesn't prevent multiple
/// `operation_state`s from entering their own critical section independently
/// (the section where message operation actually occurs).
template<async_operation_state_config CONFIG, STDEXEC::receiver Receiver>
struct zmq_async_message_operation_state : immovable
{
  private /* alias and defines */:
    using op_mode_t = async_operation_state_config::mode_t;

    /// \brief Alias to either `wait_write` or `wait_read`, depending
    /// on the operation mode. Refer to `CONFIG`.
    static constexpr auto WAIT_TYPE = [] {
        if constexpr (CONFIG.mode == op_mode_t::send)
            return native_fd_watcher_t::wait_write;
        else if constexpr (CONFIG.mode == op_mode_t::receive)
            return native_fd_watcher_t::wait_read;
        else {
            static_assert(false, "ZeroMQ internal error: Unknown operation mode");
        }
    }();

    /// \brief Alias to either `zmq::send_flags` or `zmq::recv_flags`, depending
    /// on the operation mode. Refer to `CONFIG`.
    using op_flags_t = decltype([] {
        if constexpr (CONFIG.mode == op_mode_t::send)
            return zmq::send_flags{};
        else if constexpr (CONFIG.mode == op_mode_t::receive)
            return zmq::recv_flags{};
        else {
            static_assert(false, "ZeroMQ internal error: Unknown operation mode");
        }
    }());

    /// \brief Cancellation callback struct
    struct cancel_cb
    {
        zmq_async_message_operation_state &self;
        void operator()() noexcept
        {
            // If no other threads have entered critical section, then this
            // cancellation had just entered the critical section here. In this case,
            // cancel the file descriptor wakeup.
            if (!self.m_ready.exchange(true, std::memory_order_acq_rel)) {
                self.m_watcher.cancel();
                STDEXEC::set_stopped(std::move(self.m_rec));
                // Unsubscribe any stop callback as the task is now stopped.
                self.m_stop_callback.reset();
            }
            // Otherwise: Do nothing, as the message operation had already been
            // performed in another thread.
        }
    };

    /// \note Refer to
    /// https://www.open-std.org/jtc1/sc22/wg21/docs/papers/2024/p2300r10.html#example-async-windows-socket-recv
    using stop_callback_t = STDEXEC::stop_callback_for_t<
      STDEXEC::stop_token_of_t<STDEXEC::env_of_t<Receiver>>,
      cancel_cb>;

    /// \brief Dummy type to forcefully make the constructors lazily instantiated.
    /// \note This is used to make sure `static_assert(false)` work as intended.
    struct lazy_eval
    {
    };

  private /* field */:
    /// \note stdexec receiver
    Receiver m_rec;
    native_fd_watcher_t &m_watcher;
    zmq::socket_t &m_socket;
    zmq::message_t m_message;
    op_flags_t m_flags;
    std::optional<stop_callback_t> m_stop_callback;
    /// \brief The ZeroMQ message should only be sent or received once per
    /// operation. This is tracked by this variable. The memory barrier is
    /// required to guarantee thread safety when the operation is rescheduled on
    /// different threads.
    ///
    /// This is a oneshot flag. It's only ever set from `false` to `true` once.
    ///
    /// This flag is set to `true` when one of the threads had declared
    /// responsibility for completing (either successfully or unsuccessfully) the
    /// message operation:
    ///
    /// - For send/receive/receive_multipart, this means the thread had observed
    /// poll-in/poll-out events, preventing other threads from entering the
    /// critical section.
    /// - For send_multipart, this means the thread have either observed error
    /// during send, or had already submitted last piece of the message into send
    /// queue.
    ///
    /// This can be seen as the protection flag for the per-operation critical
    /// section.
    std::atomic<bool> m_ready = false;

  public /* ctors */:
    /// \brief The constructor for send operation, where the socket does not have
    /// multipart message support.
    template<lazy_eval = {}>
    inline zmq_async_message_operation_state(Receiver rec,
                                             native_fd_watcher_t &watcher,
                                             zmq::socket_t &socket,
                                             zmq::message_t message,
                                             op_flags_t flags) :
        m_rec(std::move(rec)),
        m_watcher(watcher),
        m_socket(socket),
        m_message(std::move(message)),
        m_flags(flags)
    {
        compile_time_fact_message_parameter_supplied();
    }

    /// \brief The constructor for receive operation, where the socket does not
    /// have multipart message support.
    template<lazy_eval = {}>
    inline zmq_async_message_operation_state(Receiver rec,
                                             native_fd_watcher_t &watcher,
                                             zmq::socket_t &socket,
                                             op_flags_t flags) :
        m_rec(std::move(rec)), m_watcher(watcher), m_socket(socket), m_flags(flags)
    {
        compile_time_fact_message_parameter_NOT_supplied();
    }

  private /* compile time helpers */:
    template<lazy_eval = {}> void compile_time_fact_message_parameter_supplied()
    {
        static_assert(CONFIG.mode == op_mode_t::send,
                      "ZeroMQ internal error: zmq::message_t should only be "
                      "supplied in send operation");
    }

    template<lazy_eval = {}> void compile_time_fact_message_parameter_NOT_supplied()
    {
        static_assert(CONFIG.mode == op_mode_t::receive,
                      "ZeroMQ internal error: zmq::message_t should only be left "
                      "blank in receive operation");
    }

  private /* helpers */:
    [[nodiscard]] inline bool has_pollout_events() const noexcept
    {
        return m_socket.get(zmq::sockopt::events) & ZMQ_POLLOUT;
    }

    [[nodiscard]] inline bool has_pollin_events() const noexcept
    {
        return m_socket.get(zmq::sockopt::events) & ZMQ_POLLIN;
    }

    /// \brief Alias to either `has_pollout_events` or `has_pollin_events`,
    /// depending on the operation mode. Refer to `CONFIG`.
    /// \note To avoid confusion, this is strictly an alias. It does not attempt
    /// to deduplicate any code implementations.
    [[nodiscard]] inline bool has_events() const noexcept
    {
        if constexpr (CONFIG.mode == op_mode_t::send)
            return has_pollout_events();
        else if constexpr (CONFIG.mode == op_mode_t::receive)
            return has_pollin_events();
        else {
            static_assert(false, "ZeroMQ internal error: Unknown operation mode");
            std::abort();
        }
    }

    /// \brief Schedule for wakeup on events come in at the file descriptor using
    /// Asio.
    inline void schedule_wakeup() noexcept
    {
        m_watcher.async_wait(WAIT_TYPE, [this](boost::system::error_code err) {
            if (!err) {
                if (m_socket)
                    msg_operation();
                else
                    error_null_handle();
            }
        });
    }

  private /* operation */:
    /// \brief single zmq::message_t send operation, without any checks
    /// \pre m_socket != nullptr
    inline void msg_send_unsafe() noexcept
        requires(CONFIG.mode == op_mode_t::send)
    {
        auto result =
          m_socket.send(std::move(m_message), zmq::send_flags::dontwait | m_flags);
        if (result) [[likely]] {
            STDEXEC::set_value(std::move(m_rec));
        } else [[unlikely]] {
            STDEXEC::set_error(std::move(m_rec),
                               socket_error_t{socket_error_t::send_error});
        }
    }

    /// \brief single zmq::message_t receive operation, without any checks
    /// \pre m_socket != nullptr
    inline void msg_recv_unsafe() noexcept
        requires(CONFIG.mode == op_mode_t::receive)
    {
        auto result = m_socket.recv(m_message, zmq::recv_flags::dontwait | m_flags);
        if (result) [[likely]] {
            STDEXEC::set_value(std::move(m_rec), std::move(m_message));
        } else [[unlikely]] {
            STDEXEC::set_error(std::move(m_rec),
                               socket_error_t{socket_error_t::recv_error});
        }
    }

    /// \brief Alias to either `msg_send_unsafe` or `msg_recv_unsafe`,
    /// depending on the operation mode. Refer to `CONFIG`.
    /// \pre m_socket != nullptr
    /// \note To avoid confusion, this is strictly an alias. It does not attempt
    /// to deduplicate any code implementations.
    inline void msg_operation_unsafe() noexcept
    {
        if constexpr (CONFIG.mode == op_mode_t::send)
            return msg_send_unsafe();
        else if constexpr (CONFIG.mode == op_mode_t::receive)
            return msg_recv_unsafe();
        else {
            static_assert(false, "ZeroMQ internal error: Unknown operation mode");
            std::abort();
        }
    }

  private /* callback */:
    /// \brief single zmq::message_t send operation
    /// \pre m_socket != nullptr
    inline void msg_send() noexcept
        requires(CONFIG.mode == op_mode_t::send)
    {
        if (has_events()) {
            // If the previous state of `m_ready` is true, it means another thread
            // had entered the critical section before. Do nothing in this thread.
            if (m_ready.exchange(true, std::memory_order_acq_rel))
                return;
            // Otherwise: This thread had just entered the critical section. Perform
            // the message operation.
            msg_operation_unsafe();
            m_stop_callback.reset();
        } else {
            schedule_wakeup();
        }
    }

    /// \brief single zmq::message_t receive operation
    /// \pre m_socket != nullptr
    inline void msg_recv() noexcept
        requires(CONFIG.mode == op_mode_t::receive)
    {
        if (has_events()) {
            // If the previous state of `m_ready` is true, it means another thread had
            // entered the critical section before. Do nothing in this thread.
            if (m_ready.exchange(true, std::memory_order_acq_rel))
                return;
            // Otherwise: This thread had just entered the critical section. Perform
            // the message operation.
            msg_operation_unsafe();
            m_stop_callback.reset();
        } else {
            schedule_wakeup();
        }
    }

    /// \brief Alias to either `msg_send` or `msg_recv`,
    /// depending on the operation mode. Refer to `CONFIG`.
    /// \pre m_socket != nullptr
    /// \note To avoid confusion, this is strictly an alias. It does not attempt
    /// to deduplicate any code implementations.
    inline void msg_operation() noexcept
    {
        if constexpr (CONFIG.mode == op_mode_t::send)
            return msg_send();
        else if constexpr (CONFIG.mode == op_mode_t::receive)
            return msg_recv();
        else {
            static_assert(false, "ZeroMQ internal error: Unknown operation mode");
            std::abort();
        }
    }

    /// \brief Set the null socket error
    /// \pre m_socket == nullptr
    inline void error_null_handle() noexcept
    {
        STDEXEC::set_error(std::move(m_rec),
                           socket_error_t{socket_error_t::null_socket});
        m_stop_callback.reset();
    }

  public /* stdexec */:
    inline void start() noexcept
    {
        // Hot loop optimization: If it can be immediately processed, skip any
        // atomic fetching steps and just directly send/receive.
        // Note this does not set the `m_ready` flag, but it's OK since that
        // flag is only used to prevent race condition in threading, and
        // this starts the message operation on current thread.
        //
        // Since all multipart operations require repeatedly calling send/recv,
        // this optimization is necessary to avoid excessive atomic fetching.
        if (m_socket) {
            if (has_events()) {
                msg_operation_unsafe();
                return;
            }
        } else {
            error_null_handle();
            return;
        }

        // Refer to
        // https://www.open-std.org/jtc1/sc22/wg21/docs/papers/2024/p2300r10.html#example-async-windows-socket-recv
        auto st = STDEXEC::get_stop_token(STDEXEC::get_env(m_rec));
        // if the operation has already been cancelled, don't bother with subsequent
        // operations.
        if (st.stop_requested()) {
            STDEXEC::set_stopped(std::move(m_rec));
            return;
        }

        // Store and cache result here in case it changes during execution
        const bool stop_possible = st.stop_possible();

        // Start the message operation.
        if (m_socket) {
            msg_operation();
        } else {
            error_null_handle();
            return;
        }

        if (stop_possible) {
            // There used to be a `unsafe_atomic` but it's deprecated by the new
            // hot loop optimization. Refer to the top of `start()` for details.
            //
            // Register the stop callback
            m_stop_callback.emplace(std::move(st), cancel_cb{*this});
            // One potential outcome is that the message operation got scheduled and
            // completed very fast, before the stop callback was even registered. In
            // that case, the completing thread won't observe the registered
            // callback and won't unregister it.
            if (m_ready.load(std::memory_order_acquire)) {
                // If that ever happens, unregister the callback here. But don't call
                // set_value/set_stop here, as that will be eventually handled by the
                // completing thread.
                m_stop_callback.reset();
            }
        }
    }
};

/// \brief The Sender of a ZeroMQ message send operation.
///
/// \note Don't be confused by the name. The first `send` represents the ZeroMQ
/// message send operation. The last `sender` represents that this struct is a
/// Sender under C++26 std::execution.
struct zmq_async_send_sender
{
    using sender_concept = STDEXEC::sender_t;
    using completion_signatures =
      STDEXEC::completion_signatures<STDEXEC::set_value_t(),
                                     STDEXEC::set_error_t(socket_error_t),
                                     STDEXEC::set_stopped_t()>;

    native_fd_watcher_t &m_watcher;
    zmq::socket_t &m_socket;
    zmq::message_t m_message;
    zmq::send_flags m_flags;

    template<STDEXEC::receiver Receiver> auto connect(Receiver &&receiver) &&
    {
        return zmq_async_message_operation_state                           //
          <{.mode = async_operation_state_config::mode_t::send}, Receiver> //
          {auto{std::move(receiver)}, std::ref(m_watcher), std::ref(m_socket),
           auto{std::move(m_message)}, auto{m_flags}};
    }
};

/// \brief The Sender of a ZeroMQ message receive operation.
///
/// \note Don't be confused by the name. The first `recv` represents the ZeroMQ
/// message receive operation. The last `sender` represents that this struct is
/// a Sender under C++26 std::execution.
struct zmq_async_recv_sender
{
    using sender_concept = STDEXEC::sender_t;
    using completion_signatures =
      STDEXEC::completion_signatures<STDEXEC::set_value_t(zmq::message_t),
                                     STDEXEC::set_error_t(socket_error_t),
                                     STDEXEC::set_stopped_t()>;

    native_fd_watcher_t &m_watcher;
    zmq::socket_t &m_socket;
    zmq::recv_flags m_flags;

    template<STDEXEC::receiver Receiver> auto connect(Receiver &&receiver) &&
    {
        return zmq_async_message_operation_state                              //
          <{.mode = async_operation_state_config::mode_t::receive}, Receiver> //
          {auto{std::move(receiver)}, std::ref(m_watcher), std::ref(m_socket),
           auto{m_flags}};
    }
};

} // namespace details

/// \brief ZeroMQ socket type, with asynchronous support for send/receive operations.
///
template<auto... socket_type> struct async_socket_t
{
    static_assert(false, R"(Invalid template parameters supplied.
Note: You can use this socket in two ways:

1. Supply it with an enum value from `zmq::socket_type`:

```cpp
auto socket = async_socket_t<socket_type::req> {io-scheduler, zmq-context};
```

2. Or, leave the template parameter blank, to use the type-erased socket:

```cpp
auto socket = async_socket_t<> {io-scheduler, zmq-context, socket_type::req};
```
)");
};

///
/// \brief Type-erased ZeroMQ asynchronous socket type. It has the same interface as
/// `async_socket_t<socket-type>`.
///
/// \example
///
/// ```cpp
///
/// using zmq::async::v1::async_socket_t, zmq::socket_type;
/// using namespace std::string_literals;
///
/// boost::asio::thread_pool pool;
/// zmq::context_t ctx;
/// async_socket_t<> socket{pool.get_executor(), ctx, socket_type::req};
/// co_await socket.send(zmq::message_t{"Hi"s});
///
/// ```
///
/// In a synchronous context:
///
/// ```cpp
///
/// namespace ex = std::execution;
/// ...
/// async_socket_t<> socket{ ... };
/// ex::sync_wait(socket.send(zmq::message_t{"Hi"s})).value();
///
/// ```
///
/// \note If you prefer the socket type to be determined at compile time instead,
/// use `async_socket_t<socket-type>` instead, such as `async_socket_t<socket_type::req>`.
///
template<> struct async_socket_t<>
{
  public /* ctors */:
    inline async_socket_t(boost::asio::any_io_executor executor,
                          zmq::context_t &ctx,
                          zmq::socket_type type) :
        m_socket(ctx, type), m_watcher(executor, m_socket.get(zmq::sockopt::fd))
    {
    }
    async_socket_t(async_socket_t &&) noexcept = default;
    async_socket_t &operator=(async_socket_t &&) noexcept = default;
    async_socket_t(async_socket_t const &) = delete;
    async_socket_t &operator=(async_socket_t const &) = delete;
    inline ~async_socket_t() { m_watcher.release(); }

  public /* interface */:
    [[nodiscard(macro_discarded_sender_diagnose_message)]]
    inline auto send(zmq::message_t msg,
                     zmq::send_flags flags = zmq::send_flags::none)
      -> details::zmq_async_send_sender
    {
        return details::zmq_async_send_sender{std::ref(m_watcher),
                                              std::ref(m_socket),
                                              auto{std::move(msg)}, auto{flags}};
    }

    [[nodiscard(macro_discarded_sender_diagnose_message)]]
    inline auto recv(zmq::recv_flags flags = zmq::recv_flags::none)
      -> details::zmq_async_recv_sender
    {
        return details::zmq_async_recv_sender{std::ref(m_watcher),
                                              std::ref(m_socket), auto{flags}};
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
    inline void set(sockopt::integral_option<Opt, T, BoolUnit> _, const T &val)
    {
        m_socket.set(_, val);
    }

    // Set integral socket option from boolean, e.g.
    // `socket.set(zmq::sockopt::immediate, false)`
    template<int Opt, class T>
    inline void set(sockopt::integral_option<Opt, T, true> _, bool val)
    {
        m_socket.set(_, val);
    }

    // Set array socket option, e.g.
    // `socket.set(zmq::sockopt::plain_username, "foo123")`
    template<int Opt, int NullTerm>
    inline void set(sockopt::array_option<Opt, NullTerm> _, const char *buf)
    {
        m_socket.set(_, buf);
    }

    // Set array socket option, e.g.
    // `socket.set(zmq::sockopt::routing_id, zmq::buffer(id))`
    template<int Opt, int NullTerm>
    inline void set(sockopt::array_option<Opt, NullTerm> _, const_buffer buf)
    {
        m_socket.set(_, buf);
    }

    // Set array socket option, e.g.
    // `socket.set(zmq::sockopt::routing_id, id_str)`
    template<int Opt, int NullTerm>
    inline void set(sockopt::array_option<Opt, NullTerm> _, const std::string &buf)
    {
        m_socket.set(_, buf);
    }

    // Set array socket option, e.g.
    // `socket.set(zmq::sockopt::routing_id, id_str)`
    template<int Opt, int NullTerm>
    inline void set(sockopt::array_option<Opt, NullTerm> _, std::string_view buf)
    {
        m_socket.set(_, buf);
    }

    inline decltype(auto) unbind(const char *addr) { return m_socket.unbind(addr); }
    inline decltype(auto) unbind(std::string addr)
    {
        return m_socket.unbind(std::move(addr));
    }

  public /* accessor */:
    auto &get_socket() { return m_socket; }

  private /* field */:
    zmq::socket_t m_socket;
    native_fd_watcher_t m_watcher;
};

template<zmq::socket_type socket_type>
    requires(!details::is_fd_supported(socket_type))
struct async_socket_t<socket_type>
{
    consteval async_socket_t(auto &&...) = delete(
      "This socket type does not allow acquiring file descriptor, "
      "hence no asynchronous support.\n"
      "Refer to https://github.com/zeromq/libzmq/issues/2941");

    static_assert(false,
                  "This socket type does not allow acquiring file descriptor, "
                  "hence no asynchronous support.\n"
                  "Refer to https://github.com/zeromq/libzmq/issues/2941");
};

///
/// \brief The ZeroMQ socket type, with asynchronous support for send/receive operations.
///
/// \example
///
/// In a coroutine context:
///
/// ```cpp
///
/// using zmq::async::v1::async_socket_t, zmq::socket_type;
/// using namespace std::string_literals;
///
/// boost::asio::thread_pool pool;
/// zmq::context_t ctx;
/// async_socket_t<socket_type::req> socket{pool.get_executor(), ctx};
/// co_await socket.send(zmq::message_t{"Hi"s});
///
/// ```
///
/// In a synchronous context:
///
/// ```cpp
///
/// namespace ex = std::execution;
/// ...
/// async_socket_t<socket_type::req> socket{ ... };
/// ex::sync_wait(socket.send(zmq::message_t{"Hi"s})).value();
///
/// ```
///
/// \note If you prefer the socket type to be determined at runtime time instead, or
/// you are writing generic function but does not care about the actual socket type,
/// use `async_socket_t<>` instead (taking no template parameter).
///
template<zmq::socket_type socket_type>
    requires(details::is_fd_supported(socket_type))
struct async_socket_t<socket_type> : async_socket_t<>
{
    inline async_socket_t(boost::asio::any_io_executor executor,
                          zmq::context_t &ctx) :
        async_socket_t<>(std::move(executor), ctx, socket_type)
    {
    }
    async_socket_t(async_socket_t &&) noexcept = default;
    async_socket_t &operator=(async_socket_t &&) noexcept = default;
    async_socket_t(async_socket_t const &) = delete;
    async_socket_t &operator=(async_socket_t const &) = delete;
};

// Deduction guide for type-erased socket
async_socket_t(boost::asio::any_io_executor, zmq::context_t &, zmq::socket_type)
  -> async_socket_t<>;

///
/// \brief Send a series of messages as an atomic multipart message to
/// the given socket.
/// \param msg - The span over the messages to be sent.
/// \throws socket_error_t - If an error occured during the operation.
/// \note The user should make sure there are only one actor receiving
/// messages from this socket, as multipart message operations are not
/// concurrent-safe.
///
[[nodiscard(macro_discarded_sender_diagnose_message)]]
inline auto send_multipart(async_socket_t<> &socket,
                           std::span<zmq::message_t> msg,
                           zmq::send_flags flags = zmq::send_flags::none)
  -> exec::task<void>
{
    // Do nothing for empty container
    if (msg.empty()) {
        co_return;
    }

    // The user should not control the `sndmore` flag.
    flags = flags & ~(zmq::send_flags::sndmore);
    for (auto i = 0uz; i < msg.size() - 1; ++i) {
        co_await socket.send(std::move(msg[i]), zmq::send_flags::sndmore | flags);
    }
    co_await socket.send(std::move(msg.back()), flags);

    co_return;
}

///
/// \brief Send a multipart message to the given socket.
/// \param msg - The multipart message to be sent.
/// \throws socket_error_t - If an error occured during the operation.
/// \note The user should make sure there are only one actor receiving
/// messages from this socket, as multipart message operations are not
/// concurrent-safe.
///
[[nodiscard(macro_discarded_sender_diagnose_message)]]
inline auto send_multipart(async_socket_t<> &socket,
                           zmq::multipart_t msg,
                           zmq::send_flags flags = zmq::send_flags::none)
  -> exec::task<void>
{
    // Do nothing for empty container
    if (msg.empty()) {
        co_return;
    }

    // The user should not control the `sndmore` flag.
    flags = flags & ~(zmq::send_flags::sndmore);

    // Store size as calling `pop` causes the size to change.
    const auto size = msg.size();
    for (auto i = 0uz; i < size - 1; ++i) {
        co_await socket.send(msg.pop(), zmq::send_flags::sndmore | flags);
    }
    co_await socket.send(msg.pop(), flags);

    co_return;
}

///
/// \brief Send a series of messages as an atomic multipart message to
/// the given socket.
/// \param msg - The span over the messages to be sent.
/// \throws socket_error_t - If an error occured during the operation.
/// \note The user should make sure there are only one actor receiving
/// messages from this socket, as multipart message operations are not
/// concurrent-safe.
///
template<zmq::socket_type socket_type>
[[nodiscard(macro_discarded_sender_diagnose_message)]]
inline auto send_multipart(async_socket_t<socket_type> &socket,
                           std::span<zmq::message_t> msg,
                           zmq::send_flags flags = zmq::send_flags::none)
  -> exec::task<void>
    requires(details::is_multipart_supported(socket_type))
{
    return send_multipart(static_cast<async_socket_t<> &>(socket), msg, flags);
}

template<zmq::socket_type socket_type>
auto send_multipart(async_socket_t<socket_type> &socket,
                    std::span<zmq::message_t> msg,
                    zmq::send_flags flags = zmq::send_flags::none)
    requires(!details::is_multipart_supported(socket_type))
= delete("This socket type does not support multipart message.");

[[nodiscard(macro_discarded_sender_diagnose_message)]]
inline auto recv_multipart(async_socket_t<> &socket,
                           zmq::recv_flags flags = zmq::recv_flags::none)
  -> exec::task<zmq::multipart_t>
{
    zmq::multipart_t msg;
    while (true) {
        auto m = co_await socket.recv(flags);
        if (m.more()) {
            msg.add(std::move(m));
            continue;
        } else {
            msg.add(std::move(m));
            break;
        }
    }
    co_return msg;
}

///
/// \brief Receive a multipart message from the given socket
/// \return The received message
/// \throws socket_error_t - If an error occured during the operation.
/// \note The user should make sure there are only one actor receiving
/// messages from this socket, as multipart message operations are not
/// concurrent-safe.
///
template<zmq::socket_type socket_type>
[[nodiscard(macro_discarded_sender_diagnose_message)]]
inline auto recv_multipart(async_socket_t<socket_type> &socket,
                           zmq::recv_flags flags = zmq::recv_flags::none)
  -> exec::task<zmq::multipart_t>
    requires(details::is_multipart_supported(socket_type))
{
    return recv_multipart(static_cast<async_socket_t<> &>(socket), flags);
}

template<zmq::socket_type socket_type>
auto recv_multipart(async_socket_t<socket_type> &socket,
                    zmq::recv_flags flags = zmq::recv_flags::none)
    requires(!details::is_multipart_supported(socket_type))
= delete("This socket type does not support multipart message.");

///
/// \brief Establish two-way message forwarding between the supplied sockets. It's
/// cancellation-safe and asynchronous.
///
/// \note for implementation guidelines: this does not issue call to `zmq_proxy` as
/// it's inherently blocking and doesn't have good support for cancellation.
///
[[nodiscard(macro_discarded_sender_diagnose_message)]]
inline auto proxy(async_socket_t<> frontend_socket, async_socket_t<> backend_socket)
  -> exec::task<void>
{
    co_await exec::when_any(
      [&] -> exec::task<void> {
          for (;;) {
              auto msg = co_await recv_multipart(frontend_socket);
              co_await send_multipart(backend_socket, std::move(msg));
          }
      }(),
      [&] -> exec::task<void> {
          for (;;) {
              auto msg = co_await recv_multipart(backend_socket);
              co_await send_multipart(frontend_socket, std::move(msg));
          }
      }());
}

} // namespace zmq::async::v1

#undef macro_discarded_sender_diagnose_message

#endif