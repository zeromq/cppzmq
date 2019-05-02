/*
    Copyright (c) 2016-2017 ZeroMQ community
    Copyright (c) 2009-2011 250bpm s.r.o.
    Copyright (c) 2011 Botond Ballo
    Copyright (c) 2007-2009 iMatix Corporation

    Permission is hereby granted, free of charge, to any person obtaining a copy
    of this software and associated documentation files (the "Software"), to
    deal in the Software without restriction, including without limitation the
    rights to use, copy, modify, merge, publish, distribute, sublicense, and/or
    sell copies of the Software, and to permit persons to whom the Software is
    furnished to do so, subject to the following conditions:

    The above copyright notice and this permission notice shall be included in
    all copies or substantial portions of the Software.

    THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
    IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
    FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
    AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
    LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
    FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS
    IN THE SOFTWARE.
*/

#ifndef __ZMQ_HPP_INCLUDED__
#define __ZMQ_HPP_INCLUDED__

// macros defined if has a specific standard or greater
#if (defined(__cplusplus) && __cplusplus >= 201103L) || (defined(_MSC_VER) && _MSC_VER >= 1900)
    #define ZMQ_CPP11
#endif
#if (defined(__cplusplus) && __cplusplus >= 201402L) || (defined(_HAS_CXX14) && _HAS_CXX14 == 1)
    #define ZMQ_CPP14
#endif
#if (defined(__cplusplus) && __cplusplus >= 201703L) || (defined(_HAS_CXX17) && _HAS_CXX17 == 1)
    #define ZMQ_CPP17
#endif

#if defined(ZMQ_CPP14)
#define ZMQ_DEPRECATED(msg) [[deprecated(msg)]]
#elif defined(_MSC_VER)
#define ZMQ_DEPRECATED(msg) __declspec(deprecated(msg))
#elif defined(__GNUC__)
#define ZMQ_DEPRECATED(msg) __attribute__((deprecated(msg)))
#endif

#if defined(ZMQ_CPP17)
#define ZMQ_NODISCARD [[nodiscard]]
#else
#define ZMQ_NODISCARD
#endif

#if defined(ZMQ_CPP11)
#define ZMQ_NOTHROW noexcept
#define ZMQ_EXPLICIT explicit
#define ZMQ_OVERRIDE override
#define ZMQ_NULLPTR nullptr
#define ZMQ_CONSTEXPR_FN constexpr
#define ZMQ_CONSTEXPR_VAR constexpr
#else
#define ZMQ_NOTHROW throw()
#define ZMQ_EXPLICIT
#define ZMQ_OVERRIDE
#define ZMQ_NULLPTR 0
#define ZMQ_CONSTEXPR_FN
#define ZMQ_CONSTEXPR_VAR const
#endif

#include <zmq.h>

#include <cassert>
#include <cstring>

#include <algorithm>
#include <exception>
#include <iomanip>
#include <iterator>
#include <sstream>
#include <string>
#include <vector>

/*  Version macros for compile-time API version detection                     */
#define CPPZMQ_VERSION_MAJOR 4
#define CPPZMQ_VERSION_MINOR 3
#define CPPZMQ_VERSION_PATCH 1

#define CPPZMQ_VERSION                                                              \
    ZMQ_MAKE_VERSION(CPPZMQ_VERSION_MAJOR, CPPZMQ_VERSION_MINOR,                    \
                     CPPZMQ_VERSION_PATCH)

#ifdef ZMQ_CPP11
#include <chrono>
#include <tuple>
#include <memory>
#endif

//  Detect whether the compiler supports C++11 rvalue references.
#if (defined(__GNUC__) && (__GNUC__ > 4 || (__GNUC__ == 4 && __GNUC_MINOR__ > 2))   \
     && defined(__GXX_EXPERIMENTAL_CXX0X__))
#define ZMQ_HAS_RVALUE_REFS
#define ZMQ_DELETED_FUNCTION = delete
#elif defined(__clang__)
#if __has_feature(cxx_rvalue_references)
#define ZMQ_HAS_RVALUE_REFS
#endif

#if __has_feature(cxx_deleted_functions)
#define ZMQ_DELETED_FUNCTION = delete
#else
#define ZMQ_DELETED_FUNCTION
#endif
#elif defined(_MSC_VER) && (_MSC_VER >= 1900)
#define ZMQ_HAS_RVALUE_REFS
#define ZMQ_DELETED_FUNCTION = delete
#elif defined(_MSC_VER) && (_MSC_VER >= 1600)
#define ZMQ_HAS_RVALUE_REFS
#define ZMQ_DELETED_FUNCTION
#else
#define ZMQ_DELETED_FUNCTION
#endif

#if ZMQ_VERSION >= ZMQ_MAKE_VERSION(3, 3, 0)
#define ZMQ_NEW_MONITOR_EVENT_LAYOUT
#endif

#if ZMQ_VERSION >= ZMQ_MAKE_VERSION(4, 1, 0)
#define ZMQ_HAS_PROXY_STEERABLE
/*  Socket event data  */
typedef struct
{
    uint16_t event; // id of the event as bitfield
    int32_t value;  // value is either error code, fd or reconnect interval
} zmq_event_t;
#endif

// Avoid using deprecated message receive function when possible
#if ZMQ_VERSION < ZMQ_MAKE_VERSION(3, 2, 0)
#define zmq_msg_recv(msg, socket, flags) zmq_recvmsg(socket, msg, flags)
#endif


// In order to prevent unused variable warnings when building in non-debug
// mode use this macro to make assertions.
#ifndef NDEBUG
#define ZMQ_ASSERT(expression) assert(expression)
#else
#define ZMQ_ASSERT(expression) (void) (expression)
#endif

namespace zmq
{
typedef zmq_free_fn free_fn;
typedef zmq_pollitem_t pollitem_t;

class error_t : public std::exception
{
  public:
    error_t() : errnum(zmq_errno()) {}
    virtual const char *what() const ZMQ_NOTHROW ZMQ_OVERRIDE { return zmq_strerror(errnum); }
    int num() const { return errnum; }

  private:
    int errnum;
};

inline int poll(zmq_pollitem_t *items_, size_t nitems_, long timeout_ = -1)
{
    int rc = zmq_poll(items_, static_cast<int>(nitems_), timeout_);
    if (rc < 0)
        throw error_t();
    return rc;
}

ZMQ_DEPRECATED("from 4.3.1, use poll taking non-const items")
inline int poll(zmq_pollitem_t const *items_, size_t nitems_, long timeout_ = -1)
{
    return poll(const_cast<zmq_pollitem_t *>(items_), nitems_, timeout_);
}

#ifdef ZMQ_CPP11
ZMQ_DEPRECATED("from 4.3.1, use poll taking non-const items")
inline int
poll(zmq_pollitem_t const *items, size_t nitems, std::chrono::milliseconds timeout)
{
    return poll(const_cast<zmq_pollitem_t *>(items), nitems, static_cast<long>(timeout.count()));
}

ZMQ_DEPRECATED("from 4.3.1, use poll taking non-const items")
inline int poll(std::vector<zmq_pollitem_t> const &items,
                std::chrono::milliseconds timeout)
{
    return poll(const_cast<zmq_pollitem_t *>(items.data()), items.size(), static_cast<long>(timeout.count()));
}

ZMQ_DEPRECATED("from 4.3.1, use poll taking non-const items")
inline int poll(std::vector<zmq_pollitem_t> const &items, long timeout_ = -1)
{
    return poll(const_cast<zmq_pollitem_t *>(items.data()), items.size(), timeout_);
}

inline int
poll(zmq_pollitem_t *items, size_t nitems, std::chrono::milliseconds timeout)
{
    return poll(items, nitems, static_cast<long>(timeout.count()));
}

inline int poll(std::vector<zmq_pollitem_t> &items,
                std::chrono::milliseconds timeout)
{
    return poll(items.data(), items.size(), static_cast<long>(timeout.count()));
}

inline int poll(std::vector<zmq_pollitem_t> &items, long timeout_ = -1)
{
    return poll(items.data(), items.size(), timeout_);
}
#endif


inline void version(int *major_, int *minor_, int *patch_)
{
    zmq_version(major_, minor_, patch_);
}

#ifdef ZMQ_CPP11
inline std::tuple<int, int, int> version()
{
    std::tuple<int, int, int> v;
    zmq_version(&std::get<0>(v), &std::get<1>(v), &std::get<2>(v));
    return v;
}
#endif

class message_t
{
  public:
    message_t()
    {
        int rc = zmq_msg_init(&msg);
        if (rc != 0)
            throw error_t();
    }

    explicit message_t(size_t size_)
    {
        int rc = zmq_msg_init_size(&msg, size_);
        if (rc != 0)
            throw error_t();
    }

    template<typename T> message_t(T first, T last) : msg()
    {
        typedef typename std::iterator_traits<T>::value_type value_t;

        assert(std::distance(first, last) >= 0);
        size_t const size_ = static_cast<size_t>(std::distance(first, last)) * sizeof(value_t);
        int const rc = zmq_msg_init_size(&msg, size_);
        if (rc != 0)
            throw error_t();
        std::copy(first, last, data<value_t>());
    }

    message_t(const void *data_, size_t size_)
    {
        int rc = zmq_msg_init_size(&msg, size_);
        if (rc != 0)
            throw error_t();
        memcpy(data(), data_, size_);
    }

    message_t(void *data_, size_t size_, free_fn *ffn_, void *hint_ = ZMQ_NULLPTR)
    {
        int rc = zmq_msg_init_data(&msg, data_, size_, ffn_, hint_);
        if (rc != 0)
            throw error_t();
    }

#if defined(ZMQ_BUILD_DRAFT_API) && defined(ZMQ_CPP11)
    template<typename T>
    explicit message_t(const T &msg_) : message_t(std::begin(msg_), std::end(msg_))
    {
    }
#endif

#ifdef ZMQ_HAS_RVALUE_REFS
    message_t(message_t &&rhs) : msg(rhs.msg)
    {
        int rc = zmq_msg_init(&rhs.msg);
        if (rc != 0)
            throw error_t();
    }

    message_t &operator=(message_t &&rhs) ZMQ_NOTHROW
    {
        std::swap(msg, rhs.msg);
        return *this;
    }
#endif

    ~message_t() ZMQ_NOTHROW
    {
        int rc = zmq_msg_close(&msg);
        ZMQ_ASSERT(rc == 0);
    }

    void rebuild()
    {
        int rc = zmq_msg_close(&msg);
        if (rc != 0)
            throw error_t();
        rc = zmq_msg_init(&msg);
        if (rc != 0)
            throw error_t();
    }

    void rebuild(size_t size_)
    {
        int rc = zmq_msg_close(&msg);
        if (rc != 0)
            throw error_t();
        rc = zmq_msg_init_size(&msg, size_);
        if (rc != 0)
            throw error_t();
    }

    void rebuild(const void *data_, size_t size_)
    {
        int rc = zmq_msg_close(&msg);
        if (rc != 0)
            throw error_t();
        rc = zmq_msg_init_size(&msg, size_);
        if (rc != 0)
            throw error_t();
        memcpy(data(), data_, size_);
    }

    void rebuild(void *data_, size_t size_, free_fn *ffn_, void *hint_ = ZMQ_NULLPTR)
    {
        int rc = zmq_msg_close(&msg);
        if (rc != 0)
            throw error_t();
        rc = zmq_msg_init_data(&msg, data_, size_, ffn_, hint_);
        if (rc != 0)
            throw error_t();
    }

    ZMQ_DEPRECATED("from 4.3.1, use move taking non-const reference instead")
    void move(message_t const *msg_)
    {
        int rc = zmq_msg_move(&msg, const_cast<zmq_msg_t *>(msg_->handle()));
        if (rc != 0)
            throw error_t();
    }

    void move(message_t &msg_)
    {
        int rc = zmq_msg_move(&msg, msg_.handle());
        if (rc != 0)
            throw error_t();
    }

    ZMQ_DEPRECATED("from 4.3.1, use copy taking non-const reference instead")
    void copy(message_t const *msg_)
    {
        int rc = zmq_msg_copy(&msg, const_cast<zmq_msg_t *>(msg_->handle()));
        if (rc != 0)
            throw error_t();
    }

    void copy(message_t &msg_)
    {
        int rc = zmq_msg_copy(&msg, msg_.handle());
        if (rc != 0)
            throw error_t();
    }

    bool more() const ZMQ_NOTHROW
    {
        int rc = zmq_msg_more(const_cast<zmq_msg_t *>(&msg));
        return rc != 0;
    }

    void *data() ZMQ_NOTHROW { return zmq_msg_data(&msg); }

    const void *data() const ZMQ_NOTHROW
    {
        return zmq_msg_data(const_cast<zmq_msg_t *>(&msg));
    }

    size_t size() const ZMQ_NOTHROW
    {
        return zmq_msg_size(const_cast<zmq_msg_t *>(&msg));
    }

    ZMQ_NODISCARD bool empty() const ZMQ_NOTHROW
    {
        return size() == 0u;
    }

    template<typename T> T *data() ZMQ_NOTHROW { return static_cast<T *>(data()); }

    template<typename T> T const *data() const ZMQ_NOTHROW
    {
        return static_cast<T const *>(data());
    }

    ZMQ_DEPRECATED("from 4.3.0, use operator== instead")
    bool equal(const message_t *other) const ZMQ_NOTHROW
    {
        return *this == *other;
    }

    bool operator==(const message_t &other) const ZMQ_NOTHROW
    {
        const size_t my_size = size();
        return my_size == other.size() && 0 == memcmp(data(), other.data(), my_size);
    }

    bool operator!=(const message_t &other) const ZMQ_NOTHROW
    {
        return !(*this == other);
    }

#if ZMQ_VERSION >= ZMQ_MAKE_VERSION(3, 2, 0)
    int get(int property_)
    {
        int value = zmq_msg_get(&msg, property_);
        if (value == -1)
            throw error_t();
        return value;
    }
#endif

#if ZMQ_VERSION >= ZMQ_MAKE_VERSION(4, 1, 0)
    const char *gets(const char *property_)
    {
        const char *value = zmq_msg_gets(&msg, property_);
        if (value == ZMQ_NULLPTR)
            throw error_t();
        return value;
    }
#endif

#if defined(ZMQ_BUILD_DRAFT_API) && ZMQ_VERSION >= ZMQ_MAKE_VERSION(4, 2, 0)
    uint32_t routing_id() const
    {
        return zmq_msg_routing_id(const_cast<zmq_msg_t*>(&msg));
    }

    void set_routing_id(uint32_t routing_id)
    {
        int rc = zmq_msg_set_routing_id(&msg, routing_id);
        if (rc != 0)
            throw error_t();
    }

    const char* group() const
    {
        return zmq_msg_group(const_cast<zmq_msg_t*>(&msg));
    }

    void set_group(const char* group)
    {
        int rc = zmq_msg_set_group(&msg, group);
        if (rc != 0)
            throw error_t();
    }
#endif

    /** Dump content to string. Ascii chars are readable, the rest is printed as hex.
         *  Probably ridiculously slow.
         */
    std::string str() const
    {
        // Partly mutuated from the same method in zmq::multipart_t
        std::stringstream os;

        const unsigned char *msg_data = this->data<unsigned char>();
        unsigned char byte;
        size_t size = this->size();
        int is_ascii[2] = {0, 0};

        os << "zmq::message_t [size " << std::dec << std::setw(3)
           << std::setfill('0') << size << "] (";
        // Totally arbitrary
        if (size >= 1000) {
            os << "... too big to print)";
        } else {
            while (size--) {
                byte = *msg_data++;

                is_ascii[1] = (byte >= 33 && byte < 127);
                if (is_ascii[1] != is_ascii[0])
                    os << " "; // Separate text/non text

                if (is_ascii[1]) {
                    os << byte;
                } else {
                    os << std::hex << std::uppercase << std::setw(2)
                       << std::setfill('0') << static_cast<short>(byte);
                }
                is_ascii[0] = is_ascii[1];
            }
            os << ")";
        }
        return os.str();
    }

    void swap(message_t &other) ZMQ_NOTHROW
    {
        // this assumes zmq::msg_t from libzmq is trivially relocatable
        std::swap(msg, other.msg);
    }

    ZMQ_NODISCARD zmq_msg_t *handle() ZMQ_NOTHROW { return &msg; }
    ZMQ_NODISCARD const zmq_msg_t *handle() const ZMQ_NOTHROW { return &msg; }

  private:
    //  The underlying message
    zmq_msg_t msg;

    //  Disable implicit message copying, so that users won't use shared
    //  messages (less efficient) without being aware of the fact.
    message_t(const message_t &) ZMQ_DELETED_FUNCTION;
    void operator=(const message_t &) ZMQ_DELETED_FUNCTION;
};

inline void swap(message_t &a, message_t &b) ZMQ_NOTHROW
{
    a.swap(b);
}

class context_t
{
  public:
    context_t()
    {
        ptr = zmq_ctx_new();
        if (ptr == ZMQ_NULLPTR)
            throw error_t();
    }


    explicit context_t(int io_threads_,
                              int max_sockets_ = ZMQ_MAX_SOCKETS_DFLT)
    {
        ptr = zmq_ctx_new();
        if (ptr == ZMQ_NULLPTR)
            throw error_t();

        int rc = zmq_ctx_set(ptr, ZMQ_IO_THREADS, io_threads_);
        ZMQ_ASSERT(rc == 0);

        rc = zmq_ctx_set(ptr, ZMQ_MAX_SOCKETS, max_sockets_);
        ZMQ_ASSERT(rc == 0);
    }

#ifdef ZMQ_HAS_RVALUE_REFS
    context_t(context_t &&rhs) ZMQ_NOTHROW : ptr(rhs.ptr) { rhs.ptr = ZMQ_NULLPTR; }
    context_t &operator=(context_t &&rhs) ZMQ_NOTHROW
    {
        std::swap(ptr, rhs.ptr);
        return *this;
    }
#endif

    int setctxopt(int option_, int optval_)
    {
        int rc = zmq_ctx_set(ptr, option_, optval_);
        ZMQ_ASSERT(rc == 0);
        return rc;
    }

    int getctxopt(int option_) { return zmq_ctx_get(ptr, option_); }

    ~context_t() ZMQ_NOTHROW { close(); }

    void close() ZMQ_NOTHROW
    {
        if (ptr == ZMQ_NULLPTR)
            return;

        int rc;
        do {
            rc = zmq_ctx_destroy(ptr);
        } while (rc == -1 && errno == EINTR);

        ZMQ_ASSERT(rc == 0);
        ptr = ZMQ_NULLPTR;
    }

    //  Be careful with this, it's probably only useful for
    //  using the C api together with an existing C++ api.
    //  Normally you should never need to use this.
    ZMQ_EXPLICIT operator void *() ZMQ_NOTHROW { return ptr; }

    ZMQ_EXPLICIT operator void const *() const ZMQ_NOTHROW { return ptr; }

    operator bool() const ZMQ_NOTHROW { return ptr != ZMQ_NULLPTR; }

    void swap(context_t &other) ZMQ_NOTHROW
    {
        std::swap(ptr, other.ptr);
    }

  private:
    void *ptr;

    context_t(const context_t &) ZMQ_DELETED_FUNCTION;
    void operator=(const context_t &) ZMQ_DELETED_FUNCTION;
};

inline void swap(context_t &a, context_t &b) ZMQ_NOTHROW {
    a.swap(b);
}

namespace detail
{
class socket_base
{
public:
    socket_base() ZMQ_NOTHROW : _handle(ZMQ_NULLPTR) {}
    ZMQ_EXPLICIT socket_base(void *handle) ZMQ_NOTHROW : _handle(handle) {}

    template<typename T> void setsockopt(int option_, T const &optval)
    {
        setsockopt(option_, &optval, sizeof(T));
    }

    void setsockopt(int option_, const void *optval_, size_t optvallen_)
    {
        int rc = zmq_setsockopt(_handle, option_, optval_, optvallen_);
        if (rc != 0)
            throw error_t();
    }

    void getsockopt(int option_, void *optval_, size_t *optvallen_) const
    {
        int rc = zmq_getsockopt(_handle, option_, optval_, optvallen_);
        if (rc != 0)
            throw error_t();
    }

    template<typename T> T getsockopt(int option_) const
    {
        T optval;
        size_t optlen = sizeof(T);
        getsockopt(option_, &optval, &optlen);
        return optval;
    }

    void bind(std::string const &addr) { bind(addr.c_str()); }

    void bind(const char *addr_)
    {
        int rc = zmq_bind(_handle, addr_);
        if (rc != 0)
            throw error_t();
    }

    void unbind(std::string const &addr) { unbind(addr.c_str()); }

    void unbind(const char *addr_)
    {
        int rc = zmq_unbind(_handle, addr_);
        if (rc != 0)
            throw error_t();
    }

    void connect(std::string const &addr) { connect(addr.c_str()); }

    void connect(const char *addr_)
    {
        int rc = zmq_connect(_handle, addr_);
        if (rc != 0)
            throw error_t();
    }

    void disconnect(std::string const &addr) { disconnect(addr.c_str()); }

    void disconnect(const char *addr_)
    {
        int rc = zmq_disconnect(_handle, addr_);
        if (rc != 0)
            throw error_t();
    }

    bool connected() const ZMQ_NOTHROW { return (_handle != ZMQ_NULLPTR); }

    size_t send(const void *buf_, size_t len_, int flags_ = 0)
    {
        int nbytes = zmq_send(_handle, buf_, len_, flags_);
        if (nbytes >= 0)
            return static_cast<size_t>(nbytes);
        if (zmq_errno() == EAGAIN)
            return 0;
        throw error_t();
    }

    bool send(message_t &msg_, int flags_ = 0)
    {
        int nbytes = zmq_msg_send(msg_.handle(), _handle, flags_);
        if (nbytes >= 0)
            return true;
        if (zmq_errno() == EAGAIN)
            return false;
        throw error_t();
    }

    template<typename T> bool send(T first, T last, int flags_ = 0)
    {
        zmq::message_t msg(first, last);
        return send(msg, flags_);
    }

#ifdef ZMQ_HAS_RVALUE_REFS
    bool send(message_t &&msg_, int flags_ = 0) { return send(msg_, flags_); }
#endif

    size_t recv(void *buf_, size_t len_, int flags_ = 0)
    {
        int nbytes = zmq_recv(_handle, buf_, len_, flags_);
        if (nbytes >= 0)
            return static_cast<size_t>(nbytes);
        if (zmq_errno() == EAGAIN)
            return 0;
        throw error_t();
    }

    bool recv(message_t *msg_, int flags_ = 0)
    {
        int nbytes = zmq_msg_recv(msg_->handle(), _handle, flags_);
        if (nbytes >= 0)
            return true;
        if (zmq_errno() == EAGAIN)
            return false;
        throw error_t();
    }

#if defined(ZMQ_BUILD_DRAFT_API) && ZMQ_VERSION >= ZMQ_MAKE_VERSION(4, 2, 0)
    void join(const char* group)
    {
        int rc = zmq_join(_handle, group);
        if (rc != 0)
            throw error_t();
    }

    void leave(const char* group)
    {
        int rc = zmq_leave(_handle, group);
        if (rc != 0)
            throw error_t();
    }
#endif

    ZMQ_NODISCARD void *handle() ZMQ_NOTHROW { return _handle; }
    ZMQ_NODISCARD const void *handle() const ZMQ_NOTHROW { return _handle; }

    ZMQ_EXPLICIT operator bool() const ZMQ_NOTHROW { return _handle != ZMQ_NULLPTR; }
    // note: non-const operator bool can be removed once
    // operator void* is removed from socket_t
    ZMQ_EXPLICIT operator bool() ZMQ_NOTHROW { return _handle != ZMQ_NULLPTR; }

protected:
    void *_handle;
};
} // namespace detail

#ifdef ZMQ_CPP11
enum class socket_type : int
{
    req = ZMQ_REQ,
    rep = ZMQ_REP,
    dealer = ZMQ_DEALER,
    router = ZMQ_ROUTER,
    pub = ZMQ_PUB,
    sub = ZMQ_SUB,
    xpub = ZMQ_XPUB,
    xsub = ZMQ_XSUB,
    push = ZMQ_PUSH,
    pull = ZMQ_PULL,
#if defined(ZMQ_BUILD_DRAFT_API) && ZMQ_VERSION >= ZMQ_MAKE_VERSION(4, 2, 0)
    server = ZMQ_SERVER,
    client = ZMQ_CLIENT,
    radio = ZMQ_RADIO,
    dish = ZMQ_DISH,
#endif
#if ZMQ_VERSION_MAJOR >= 4
    stream = ZMQ_STREAM,
#endif
    pair = ZMQ_PAIR
};
#endif

struct from_handle_t
{
    struct _private {}; // disabling use other than with from_handle
    ZMQ_CONSTEXPR_FN ZMQ_EXPLICIT from_handle_t(_private /*p*/) ZMQ_NOTHROW {}
};

ZMQ_CONSTEXPR_VAR from_handle_t from_handle = from_handle_t(from_handle_t::_private());

// A non-owning nullable reference to a socket.
// The reference is invalidated on socket close or destruction.
class socket_ref : public detail::socket_base
{
  public:
    socket_ref() ZMQ_NOTHROW : detail::socket_base() {}
#ifdef ZMQ_CPP11
    socket_ref(std::nullptr_t) ZMQ_NOTHROW : detail::socket_base() {}
#endif
    socket_ref(from_handle_t /*fh*/, void *handle) ZMQ_NOTHROW
        : detail::socket_base(handle) {}
};

#ifdef ZMQ_CPP11
inline bool operator==(socket_ref sr, std::nullptr_t /*p*/) ZMQ_NOTHROW
{
    return sr.handle() == nullptr;
}
inline bool operator==(std::nullptr_t /*p*/, socket_ref sr) ZMQ_NOTHROW
{
    return sr.handle() == nullptr;
}
inline bool operator!=(socket_ref sr, std::nullptr_t /*p*/) ZMQ_NOTHROW
{
    return !(sr == nullptr);
}
inline bool operator!=(std::nullptr_t /*p*/, socket_ref sr) ZMQ_NOTHROW
{
    return !(sr == nullptr);
}
#endif

inline bool operator==(socket_ref a, socket_ref b) ZMQ_NOTHROW
{
    return std::equal_to<void*>()(a.handle(), b.handle());
}
inline bool operator!=(socket_ref a, socket_ref b) ZMQ_NOTHROW
{
    return !(a == b);
}
inline bool operator<(socket_ref a, socket_ref b) ZMQ_NOTHROW
{
    return std::less<void*>()(a.handle(), b.handle());
}
inline bool operator>(socket_ref a, socket_ref b) ZMQ_NOTHROW
{
    return b < a;
}
inline bool operator<=(socket_ref a, socket_ref b) ZMQ_NOTHROW
{
    return !(a > b);
}
inline bool operator>=(socket_ref a, socket_ref b) ZMQ_NOTHROW
{
    return !(a < b);
}

} // namespace zmq

#ifdef ZMQ_CPP11
namespace std
{
template<>
struct hash<zmq::socket_ref>
{
    size_t operator()(zmq::socket_ref sr) const ZMQ_NOTHROW
    {
        return hash<void*>()(sr.handle());
    }
};
} // namespace std
#endif

namespace zmq
{

class socket_t : public detail::socket_base
{
    friend class monitor_t;

  public:
    socket_t() ZMQ_NOTHROW
      : detail::socket_base(ZMQ_NULLPTR)
      , ctxptr(ZMQ_NULLPTR)
    {
    }

    socket_t(context_t &context_, int type_)
        : detail::socket_base(zmq_socket(static_cast<void*>(context_), type_))
        , ctxptr(static_cast<void*>(context_))
    {
        if (_handle == ZMQ_NULLPTR)
            throw error_t();
    }

#ifdef ZMQ_CPP11
    socket_t(context_t &context_, socket_type type_)
        : socket_t(context_, static_cast<int>(type_))
    {
    }
#endif

#ifdef ZMQ_HAS_RVALUE_REFS
    socket_t(socket_t &&rhs) ZMQ_NOTHROW : detail::socket_base(rhs._handle), ctxptr(rhs.ctxptr)
    {
        rhs._handle = ZMQ_NULLPTR;
        rhs.ctxptr = ZMQ_NULLPTR;
    }
    socket_t &operator=(socket_t &&rhs) ZMQ_NOTHROW
    {
        std::swap(_handle, rhs._handle);
        return *this;
    }
#endif

    ~socket_t() ZMQ_NOTHROW { close(); }

    operator void *() ZMQ_NOTHROW { return _handle; }

    operator void const *() const ZMQ_NOTHROW { return _handle; }

    void close() ZMQ_NOTHROW
    {
        if (_handle == ZMQ_NULLPTR)
            // already closed
            return;
        int rc = zmq_close(_handle);
        ZMQ_ASSERT(rc == 0);
        _handle = ZMQ_NULLPTR;
    }

    void swap(socket_t &other) ZMQ_NOTHROW
    {
        std::swap(_handle, other._handle);
        std::swap(ctxptr, other.ctxptr);
    }

    operator socket_ref() ZMQ_NOTHROW
    {
        return socket_ref(from_handle, _handle);
    }

  private:
    void *ctxptr;

    socket_t(const socket_t &) ZMQ_DELETED_FUNCTION;
    void operator=(const socket_t &) ZMQ_DELETED_FUNCTION;

    // used by monitor_t
    socket_t(void *context_, int type_)
        : detail::socket_base(zmq_socket(context_, type_))
        , ctxptr(context_)
    {
        if (_handle == ZMQ_NULLPTR)
            throw error_t();
    }
};

inline void swap(socket_t &a, socket_t &b) ZMQ_NOTHROW {
    a.swap(b);
}

ZMQ_DEPRECATED("from 4.3.1, use proxy taking socket_t objects")
inline void proxy(void *frontend, void *backend, void *capture)
{
    int rc = zmq_proxy(frontend, backend, capture);
    if (rc != 0)
        throw error_t();
}

inline void
proxy(socket_ref frontend, socket_ref backend, socket_ref capture = socket_ref())
{
    int rc = zmq_proxy(frontend.handle(), backend.handle(), capture.handle());
    if (rc != 0)
        throw error_t();
}

#ifdef ZMQ_HAS_PROXY_STEERABLE
ZMQ_DEPRECATED("from 4.3.1, use proxy_steerable taking socket_t objects")
inline void
proxy_steerable(void *frontend, void *backend, void *capture, void *control)
{
    int rc = zmq_proxy_steerable(frontend, backend, capture, control);
    if (rc != 0)
        throw error_t();
}

inline void proxy_steerable(socket_ref frontend,
                            socket_ref backend,
                            socket_ref capture,
                            socket_ref control)
{
    int rc = zmq_proxy_steerable(frontend.handle(), backend.handle(),
                                 capture.handle(), control.handle());
    if (rc != 0)
        throw error_t();
}
#endif

class monitor_t
{
  public:
    monitor_t() : _socket(), _monitor_socket() {}

    virtual ~monitor_t()
    {
        close();
    }

#ifdef ZMQ_HAS_RVALUE_REFS
    monitor_t(monitor_t &&rhs) ZMQ_NOTHROW : _socket(), _monitor_socket()
    {
        std::swap(_socket, rhs._socket);
        std::swap(_monitor_socket, rhs._monitor_socket);
    }

    monitor_t &operator=(monitor_t &&rhs) ZMQ_NOTHROW
    {
        close();
        _socket = socket_ref();
        std::swap(_socket, rhs._socket);
        std::swap(_monitor_socket, rhs._monitor_socket);
        return *this;
    }
#endif


    void
    monitor(socket_t &socket, std::string const &addr, int events = ZMQ_EVENT_ALL)
    {
        monitor(socket, addr.c_str(), events);
    }

    void monitor(socket_t &socket, const char *addr_, int events = ZMQ_EVENT_ALL)
    {
        init(socket, addr_, events);
        while (true) {
            check_event(-1);
        }
    }

    void init(socket_t &socket, std::string const &addr, int events = ZMQ_EVENT_ALL)
    {
        init(socket, addr.c_str(), events);
    }

    void init(socket_t &socket, const char *addr_, int events = ZMQ_EVENT_ALL)
    {
        int rc = zmq_socket_monitor(socket.handle(), addr_, events);
        if (rc != 0)
            throw error_t();

        _socket = socket;
        _monitor_socket = socket_t(socket.ctxptr, ZMQ_PAIR);
        _monitor_socket.connect(addr_);

        on_monitor_started();
    }

    bool check_event(int timeout = 0)
    {
        assert(_monitor_socket);

        zmq_msg_t eventMsg;
        zmq_msg_init(&eventMsg);

        zmq::pollitem_t items[] = {
          {_monitor_socket.handle(), 0, ZMQ_POLLIN, 0},
        };

        zmq::poll(&items[0], 1, timeout);

        if (items[0].revents & ZMQ_POLLIN) {
            int rc = zmq_msg_recv(&eventMsg, _monitor_socket.handle(), 0);
            if (rc == -1 && zmq_errno() == ETERM)
                return false;
            assert(rc != -1);

        } else {
            zmq_msg_close(&eventMsg);
            return false;
        }

#if ZMQ_VERSION_MAJOR >= 4
        const char *data = static_cast<const char *>(zmq_msg_data(&eventMsg));
        zmq_event_t msgEvent;
        memcpy(&msgEvent.event, data, sizeof(uint16_t));
        data += sizeof(uint16_t);
        memcpy(&msgEvent.value, data, sizeof(int32_t));
        zmq_event_t *event = &msgEvent;
#else
        zmq_event_t *event = static_cast<zmq_event_t *>(zmq_msg_data(&eventMsg));
#endif

#ifdef ZMQ_NEW_MONITOR_EVENT_LAYOUT
        zmq_msg_t addrMsg;
        zmq_msg_init(&addrMsg);
        int rc = zmq_msg_recv(&addrMsg, _monitor_socket.handle(), 0);
        if (rc == -1 && zmq_errno() == ETERM) {
            zmq_msg_close(&eventMsg);
            return false;
        }

        assert(rc != -1);
        const char *str = static_cast<const char *>(zmq_msg_data(&addrMsg));
        std::string address(str, str + zmq_msg_size(&addrMsg));
        zmq_msg_close(&addrMsg);
#else
        // Bit of a hack, but all events in the zmq_event_t union have the same layout so this will work for all event types.
        std::string address = event->data.connected.addr;
#endif

#ifdef ZMQ_EVENT_MONITOR_STOPPED
        if (event->event == ZMQ_EVENT_MONITOR_STOPPED) {
            zmq_msg_close(&eventMsg);
            return false;
        }

#endif

        switch (event->event) {
            case ZMQ_EVENT_CONNECTED:
                on_event_connected(*event, address.c_str());
                break;
            case ZMQ_EVENT_CONNECT_DELAYED:
                on_event_connect_delayed(*event, address.c_str());
                break;
            case ZMQ_EVENT_CONNECT_RETRIED:
                on_event_connect_retried(*event, address.c_str());
                break;
            case ZMQ_EVENT_LISTENING:
                on_event_listening(*event, address.c_str());
                break;
            case ZMQ_EVENT_BIND_FAILED:
                on_event_bind_failed(*event, address.c_str());
                break;
            case ZMQ_EVENT_ACCEPTED:
                on_event_accepted(*event, address.c_str());
                break;
            case ZMQ_EVENT_ACCEPT_FAILED:
                on_event_accept_failed(*event, address.c_str());
                break;
            case ZMQ_EVENT_CLOSED:
                on_event_closed(*event, address.c_str());
                break;
            case ZMQ_EVENT_CLOSE_FAILED:
                on_event_close_failed(*event, address.c_str());
                break;
            case ZMQ_EVENT_DISCONNECTED:
                on_event_disconnected(*event, address.c_str());
                break;
#ifdef ZMQ_BUILD_DRAFT_API
#if ZMQ_VERSION >= ZMQ_MAKE_VERSION(4, 2, 3)
            case ZMQ_EVENT_HANDSHAKE_FAILED_NO_DETAIL:
                on_event_handshake_failed_no_detail(*event, address.c_str());
                break;
            case ZMQ_EVENT_HANDSHAKE_FAILED_PROTOCOL:
                on_event_handshake_failed_protocol(*event, address.c_str());
                break;
            case ZMQ_EVENT_HANDSHAKE_FAILED_AUTH:
                on_event_handshake_failed_auth(*event, address.c_str());
                break;
            case ZMQ_EVENT_HANDSHAKE_SUCCEEDED:
                on_event_handshake_succeeded(*event, address.c_str());
                break;
#elif ZMQ_VERSION >= ZMQ_MAKE_VERSION(4, 2, 1)
            case ZMQ_EVENT_HANDSHAKE_FAILED:
                on_event_handshake_failed(*event, address.c_str());
                break;
            case ZMQ_EVENT_HANDSHAKE_SUCCEED:
                on_event_handshake_succeed(*event, address.c_str());
                break;
#endif
#endif
            default:
                on_event_unknown(*event, address.c_str());
                break;
        }
        zmq_msg_close(&eventMsg);

        return true;
    }

#ifdef ZMQ_EVENT_MONITOR_STOPPED
    void abort()
    {
        if (_socket)
            zmq_socket_monitor(_socket.handle(), ZMQ_NULLPTR, 0);

        _socket = socket_ref();
    }
#endif
    virtual void on_monitor_started() {}
    virtual void on_event_connected(const zmq_event_t &event_, const char *addr_)
    {
        (void) event_;
        (void) addr_;
    }
    virtual void on_event_connect_delayed(const zmq_event_t &event_,
                                          const char *addr_)
    {
        (void) event_;
        (void) addr_;
    }
    virtual void on_event_connect_retried(const zmq_event_t &event_,
                                          const char *addr_)
    {
        (void) event_;
        (void) addr_;
    }
    virtual void on_event_listening(const zmq_event_t &event_, const char *addr_)
    {
        (void) event_;
        (void) addr_;
    }
    virtual void on_event_bind_failed(const zmq_event_t &event_, const char *addr_)
    {
        (void) event_;
        (void) addr_;
    }
    virtual void on_event_accepted(const zmq_event_t &event_, const char *addr_)
    {
        (void) event_;
        (void) addr_;
    }
    virtual void on_event_accept_failed(const zmq_event_t &event_, const char *addr_)
    {
        (void) event_;
        (void) addr_;
    }
    virtual void on_event_closed(const zmq_event_t &event_, const char *addr_)
    {
        (void) event_;
        (void) addr_;
    }
    virtual void on_event_close_failed(const zmq_event_t &event_, const char *addr_)
    {
        (void) event_;
        (void) addr_;
    }
    virtual void on_event_disconnected(const zmq_event_t &event_, const char *addr_)
    {
        (void) event_;
        (void) addr_;
    }
#if ZMQ_VERSION >= ZMQ_MAKE_VERSION(4, 2, 3)
    virtual void on_event_handshake_failed_no_detail(const zmq_event_t &event_,
                                                     const char *addr_)
    {
        (void) event_;
        (void) addr_;
    }
    virtual void on_event_handshake_failed_protocol(const zmq_event_t &event_,
                                                    const char *addr_)
    {
        (void) event_;
        (void) addr_;
    }
    virtual void on_event_handshake_failed_auth(const zmq_event_t &event_,
                                                const char *addr_)
    {
        (void) event_;
        (void) addr_;
    }
    virtual void on_event_handshake_succeeded(const zmq_event_t &event_,
                                              const char *addr_)
    {
        (void) event_;
        (void) addr_;
    }
#elif ZMQ_VERSION >= ZMQ_MAKE_VERSION(4, 2, 1)
    virtual void on_event_handshake_failed(const zmq_event_t &event_,
                                           const char *addr_)
    {
        (void) event_;
        (void) addr_;
    }
    virtual void on_event_handshake_succeed(const zmq_event_t &event_,
                                            const char *addr_)
    {
        (void) event_;
        (void) addr_;
    }
#endif
    virtual void on_event_unknown(const zmq_event_t &event_, const char *addr_)
    {
        (void) event_;
        (void) addr_;
    }

  private:
    monitor_t(const monitor_t &) ZMQ_DELETED_FUNCTION;
    void operator=(const monitor_t &) ZMQ_DELETED_FUNCTION;

    socket_ref _socket;
    socket_t _monitor_socket;

    void close() ZMQ_NOTHROW
    {
        if (_socket)
            zmq_socket_monitor(_socket.handle(), ZMQ_NULLPTR, 0);
        _monitor_socket.close();
    }
};

#if defined(ZMQ_BUILD_DRAFT_API) && defined(ZMQ_CPP11) && defined(ZMQ_HAVE_POLLER)
template<typename T = void> class poller_t
{
  public:
    poller_t() = default;

    void add(zmq::socket_ref socket, short events, T *user_data)
    {
        if (0
            != zmq_poller_add(poller_ptr.get(), socket.handle(),
                              user_data, events)) {
            throw error_t();
        }
    }

    void remove(zmq::socket_ref socket)
    {
        if (0 != zmq_poller_remove(poller_ptr.get(), socket.handle())) {
            throw error_t();
        }
    }

    void modify(zmq::socket_ref socket, short events)
    {
        if (0
            != zmq_poller_modify(poller_ptr.get(), socket.handle(),
                                 events)) {
            throw error_t();
        }
    }

    size_t wait_all(std::vector<zmq_poller_event_t> &poller_events,
                    const std::chrono::milliseconds timeout)
    {
        int rc = zmq_poller_wait_all(poller_ptr.get(), poller_events.data(),
                                     static_cast<int>(poller_events.size()),
                                     static_cast<long>(timeout.count()));
        if (rc > 0)
            return static_cast<size_t>(rc);

#if ZMQ_VERSION >= ZMQ_MAKE_VERSION(4, 2, 3)
        if (zmq_errno() == EAGAIN)
#else
        if (zmq_errno() == ETIMEDOUT)
#endif
            return 0;

        throw error_t();
    }

  private:
    std::unique_ptr<void, void(*)(void *)> poller_ptr{
      []() {
          auto poller_new = zmq_poller_new();
          if (poller_new)
              return poller_new;
          throw error_t();
      }(), &destroy_poller};

    static void destroy_poller(void *ptr)
    {
        int rc = zmq_poller_destroy(&ptr);
        ZMQ_ASSERT(rc == 0);
    }
};
#endif //  defined(ZMQ_BUILD_DRAFT_API) && defined(ZMQ_CPP11) && defined(ZMQ_HAVE_POLLER)

inline std::ostream &operator<<(std::ostream &os, const message_t &msg)
{
    return os << msg.str();
}

} // namespace zmq

#endif // __ZMQ_HPP_INCLUDED__
