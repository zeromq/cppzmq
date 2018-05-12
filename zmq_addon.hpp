/*
    Copyright (c) 2016-2017 ZeroMQ community
    Copyright (c) 2016 VOCA AS / Harald Nøkland

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

#ifndef __ZMQ_ADDON_HPP_INCLUDED__
#define __ZMQ_ADDON_HPP_INCLUDED__

#include <zmq.hpp>

#include <deque>
#include <iomanip>
#include <sstream>
#include <stdexcept>

namespace zmq
{
#ifdef ZMQ_HAS_RVALUE_REFS

/*
    This class handles multipart messaging. It is the C++ equivalent of zmsg.h,
    which is part of CZMQ (the high-level C binding). Furthermore, it is a major
    improvement compared to zmsg.hpp, which is part of the examples in the ØMQ
    Guide. Unnecessary copying is avoided by using move semantics to efficiently
    add/remove parts.
*/
class multipart_t
{
  private:
    std::deque<message_t> m_parts;

  public:
    typedef std::deque<message_t>::iterator iterator;
    typedef std::deque<message_t>::const_iterator const_iterator;

    typedef std::deque<message_t>::reverse_iterator reverse_iterator;
    typedef std::deque<message_t>::const_reverse_iterator const_reverse_iterator;

    // Default constructor
    multipart_t() {}

    // Construct from socket receive
    multipart_t(socket_t &socket) { recv(socket); }

    // Construct from memory block
    multipart_t(const void *src, size_t size) { addmem(src, size); }

    // Construct from string
    multipart_t(const std::string &string) { addstr(string); }

    // Construct from message part
    multipart_t(message_t &&message) { add(std::move(message)); }

    // Move constructor
    multipart_t(multipart_t &&other) { m_parts = std::move(other.m_parts); }

    // Move assignment operator
    multipart_t &operator=(multipart_t &&other)
    {
        m_parts = std::move(other.m_parts);
        return *this;
    }

    // Destructor
    virtual ~multipart_t() { clear(); }

    message_t &operator[](size_t n) { return m_parts[n]; }

    const message_t &operator[](size_t n) const { return m_parts[n]; }

    message_t &at(size_t n) { return m_parts.at(n); }

    const message_t &at(size_t n) const { return m_parts.at(n); }

    iterator begin() { return m_parts.begin(); }

    const_iterator begin() const { return m_parts.begin(); }

    const_iterator cbegin() const { return m_parts.cbegin(); }

    reverse_iterator rbegin() { return m_parts.rbegin(); }

    const_reverse_iterator rbegin() const { return m_parts.rbegin(); }

    iterator end() { return m_parts.end(); }

    const_iterator end() const { return m_parts.end(); }

    const_iterator cend() const { return m_parts.cend(); }

    reverse_iterator rend() { return m_parts.rend(); }

    const_reverse_iterator rend() const { return m_parts.rend(); }

    // Delete all parts
    void clear() { m_parts.clear(); }

    // Get number of parts
    size_t size() const { return m_parts.size(); }

    // Check if number of parts is zero
    bool empty() const { return m_parts.empty(); }

    // Receive multipart message from socket
    bool recv(socket_t &socket, int flags = 0)
    {
        clear();
        bool more = true;
        while (more) {
            message_t message;
            if (!socket.recv(&message, flags))
                return false;
            more = message.more();
            add(std::move(message));
        }
        return true;
    }

    // Send multipart message to socket
    bool send(socket_t &socket, int flags = 0)
    {
        flags &= ~(ZMQ_SNDMORE);
        bool more = size() > 0;
        while (more) {
            message_t message = pop();
            more = size() > 0;
            if (!socket.send(message, (more ? ZMQ_SNDMORE : 0) | flags))
                return false;
        }
        clear();
        return true;
    }

    // Concatenate other multipart to front
    void prepend(multipart_t &&other)
    {
        while (!other.empty())
            push(other.remove());
    }

    // Concatenate other multipart to back
    void append(multipart_t &&other)
    {
        while (!other.empty())
            add(other.pop());
    }

    // Push memory block to front
    void pushmem(const void *src, size_t size)
    {
        m_parts.push_front(message_t(src, size));
    }

    // Push memory block to back
    void addmem(const void *src, size_t size)
    {
        m_parts.push_back(message_t(src, size));
    }

    // Push string to front
    void pushstr(const std::string &string)
    {
        m_parts.push_front(message_t(string.data(), string.size()));
    }

    // Push string to back
    void addstr(const std::string &string)
    {
        m_parts.push_back(message_t(string.data(), string.size()));
    }

    // Push type (fixed-size) to front
    template<typename T> void pushtyp(const T &type)
    {
        static_assert(!std::is_same<T, std::string>::value,
                      "Use pushstr() instead of pushtyp<std::string>()");
        m_parts.push_front(message_t(&type, sizeof(type)));
    }

    // Push type (fixed-size) to back
    template<typename T> void addtyp(const T &type)
    {
        static_assert(!std::is_same<T, std::string>::value,
                      "Use addstr() instead of addtyp<std::string>()");
        m_parts.push_back(message_t(&type, sizeof(type)));
    }

    // Push message part to front
    void push(message_t &&message) { m_parts.push_front(std::move(message)); }

    // Push message part to back
    void add(message_t &&message) { m_parts.push_back(std::move(message)); }

    // Pop string from front
    std::string popstr()
    {
        std::string string(m_parts.front().data<char>(), m_parts.front().size());
        m_parts.pop_front();
        return string;
    }

    // Pop type (fixed-size) from front
    template<typename T> T poptyp()
    {
        static_assert(!std::is_same<T, std::string>::value,
                      "Use popstr() instead of poptyp<std::string>()");
        if (sizeof(T) != m_parts.front().size())
            throw std::runtime_error(
              "Invalid type, size does not match the message size");
        T type = *m_parts.front().data<T>();
        m_parts.pop_front();
        return type;
    }

    // Pop message part from front
    message_t pop()
    {
        message_t message = std::move(m_parts.front());
        m_parts.pop_front();
        return message;
    }

    // Pop message part from back
    message_t remove()
    {
        message_t message = std::move(m_parts.back());
        m_parts.pop_back();
        return message;
    }

    // Get pointer to a specific message part
    const message_t *peek(size_t index) const { return &m_parts[index]; }

    // Get a string copy of a specific message part
    std::string peekstr(size_t index) const
    {
        std::string string(m_parts[index].data<char>(), m_parts[index].size());
        return string;
    }

    // Peek type (fixed-size) from front
    template<typename T> T peektyp(size_t index) const
    {
        static_assert(!std::is_same<T, std::string>::value,
                      "Use peekstr() instead of peektyp<std::string>()");
        if (sizeof(T) != m_parts[index].size())
            throw std::runtime_error(
              "Invalid type, size does not match the message size");
        T type = *m_parts[index].data<T>();
        return type;
    }

    // Create multipart from type (fixed-size)
    template<typename T> static multipart_t create(const T &type)
    {
        multipart_t multipart;
        multipart.addtyp(type);
        return multipart;
    }

    // Copy multipart
    multipart_t clone() const
    {
        multipart_t multipart;
        for (size_t i = 0; i < size(); i++)
            multipart.addmem(m_parts[i].data(), m_parts[i].size());
        return multipart;
    }

    // Dump content to string
    std::string str() const
    {
        std::stringstream ss;
        for (size_t i = 0; i < m_parts.size(); i++) {
            const unsigned char *data = m_parts[i].data<unsigned char>();
            size_t size = m_parts[i].size();

            // Dump the message as text or binary
            bool isText = true;
            for (size_t j = 0; j < size; j++) {
                if (data[j] < 32 || data[j] > 127) {
                    isText = false;
                    break;
                }
            }
            ss << "\n[" << std::dec << std::setw(3) << std::setfill('0') << size
               << "] ";
            if (size >= 1000) {
                ss << "... (to big to print)";
                continue;
            }
            for (size_t j = 0; j < size; j++) {
                if (isText)
                    ss << static_cast<char>(data[j]);
                else
                    ss << std::hex << std::setw(2) << std::setfill('0')
                       << static_cast<short>(data[j]);
            }
        }
        return ss.str();
    }

    // Check if equal to other multipart
    bool equal(const multipart_t *other) const
    {
        if (size() != other->size())
            return false;
        for (size_t i = 0; i < size(); i++)
            if (*peek(i) != *other->peek(i))
                return false;
        return true;
    }

  private:
    // Disable implicit copying (moving is more efficient)
    multipart_t(const multipart_t &other) ZMQ_DELETED_FUNCTION;
    void operator=(const multipart_t &other) ZMQ_DELETED_FUNCTION;
}; // class multipart_t

inline std::ostream &operator<<(std::ostream &os, const multipart_t &msg)
{
    return os << msg.str();
}

#endif // ZMQ_HAS_RVALUE_REFS

#if defined(ZMQ_BUILD_DRAFT_API) && defined(ZMQ_CPP11) && defined(ZMQ_HAVE_POLLER)
class active_poller_t
{
  public:
    active_poller_t() = default;
    ~active_poller_t() = default;

    active_poller_t(const active_poller_t &) = delete;
    active_poller_t &operator=(const active_poller_t &) = delete;

    active_poller_t(active_poller_t &&src) = default;
    active_poller_t &operator=(active_poller_t &&src) = default;

    using handler_t = std::function<void(short)>;

    void add(zmq::socket_t &socket, short events, handler_t handler)
    {
        auto it = decltype(handlers)::iterator{};
        auto inserted = bool{};
        std::tie(it, inserted) =
          handlers.emplace(static_cast<void *>(socket),
                           std::make_shared<handler_t>(std::move(handler)));
        try {
            base_poller.add(socket, events,
                            inserted && *(it->second) ? it->second.get() : nullptr);
            need_rebuild |= inserted;
        }
        catch (const zmq::error_t &) {
            // rollback
            if (inserted) {
                handlers.erase(static_cast<void *>(socket));
            }
            throw;
        }
    }

    void remove(zmq::socket_t &socket)
    {
        base_poller.remove(socket);
        handlers.erase(static_cast<void *>(socket));
        need_rebuild = true;
    }

    void modify(zmq::socket_t &socket, short events)
    {
        base_poller.modify(socket, events);
    }

    size_t wait(std::chrono::milliseconds timeout)
    {
        if (need_rebuild) {
            poller_events.resize(handlers.size());
            poller_handlers.clear();
            poller_handlers.reserve(handlers.size());
            for (const auto &handler : handlers) {
                poller_handlers.push_back(handler.second);
            }
            need_rebuild = false;
        }
        const auto count = base_poller.wait_all(poller_events, timeout);
        std::for_each(poller_events.begin(), poller_events.begin() + count,
                      [](zmq_poller_event_t &event) {
                          if (event.user_data != NULL)
                              (*reinterpret_cast<handler_t *>(event.user_data))(
                                event.events);
                      });
        return count;
    }

    bool empty() const { return handlers.empty(); }

    size_t size() const { return handlers.size(); }

  private:
    bool need_rebuild{false};

    poller_t<handler_t> base_poller{};
    std::unordered_map<void *, std::shared_ptr<handler_t>> handlers{};
    std::vector<zmq_poller_event_t> poller_events{};
    std::vector<std::shared_ptr<handler_t>> poller_handlers{};
};     // class active_poller_t
#endif //  defined(ZMQ_BUILD_DRAFT_API) && defined(ZMQ_CPP11) && defined(ZMQ_HAVE_POLLER)


} // namespace zmq

#endif // __ZMQ_ADDON_HPP_INCLUDED__
