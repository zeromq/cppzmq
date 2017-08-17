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

namespace zmq {

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
    multipart_t()
    {}

    // Construct from socket receive
    multipart_t(socket_t& socket)
    {
        recv(socket);
    }

    // Construct from memory block
    multipart_t(const void *src, size_t size)
    {
        addmem(src, size);
    }

    // Construct from string
    multipart_t(const std::string& string)
    {
        addstr(string);
    }

    // Construct from message part
    multipart_t(message_t&& message)
    {
        add(std::move(message));
    }

    // Move constructor
    multipart_t(multipart_t&& other)
    {
        m_parts = std::move(other.m_parts);
    }

    // Move assignment operator
    multipart_t& operator=(multipart_t&& other)
    {
        m_parts = std::move(other.m_parts);
        return *this;
    }

    // Destructor
    virtual ~multipart_t()
    {
        clear();
    }

    message_t& operator[] (size_t n)
    {
        return m_parts[n];
    }

    const message_t& operator[] (size_t n) const
    {
        return m_parts[n];
    }

    message_t& at (size_t n)
    {
        return m_parts.at(n);
    }

    const message_t& at (size_t n) const
    {
        return m_parts.at(n);
    }

    iterator begin()
    {
        return m_parts.begin();
    }

    const_iterator begin() const
    {
        return m_parts.begin();
    }

    const_iterator cbegin() const
    {
        return m_parts.cbegin();
    }

    reverse_iterator rbegin()
    {
        return m_parts.rbegin();
    }

    const_reverse_iterator rbegin() const
    {
        return m_parts.rbegin();
    }

    iterator end()
    {
        return m_parts.end();
    }

    const_iterator end() const
    {
        return m_parts.end();
    }

    const_iterator cend() const
    {
        return m_parts.cend();
    }

    reverse_iterator rend()
    {
        return m_parts.rend();
    }

    const_reverse_iterator rend() const
    {
        return m_parts.rend();
    }

    // Delete all parts
    void clear()
    {
        m_parts.clear();
    }

    // Get number of parts
    size_t size() const
    {
        return m_parts.size();
    }

    // Check if number of parts is zero
    bool empty() const
    {
        return m_parts.empty();
    }

    // Receive multipart message from socket
    bool recv(socket_t& socket, int flags = 0)
    {
        clear();
        bool more = true;
        while (more)
        {
            message_t message;
            if (!socket.recv(&message, flags))
                return false;
            more = message.more();
            add(std::move(message));
        }
        return true;
    }

    // Send multipart message to socket
    bool send(socket_t& socket, int flags = 0)
    {
        flags &= ~(ZMQ_SNDMORE);
        bool more = size() > 0;
        while (more)
        {
            message_t message = pop();
            more = size() > 0;
            if (!socket.send(message, (more ? ZMQ_SNDMORE : 0) | flags))
                return false;
        }
        clear();
        return true;
    }

    // Concatenate other multipart to front
    void prepend(multipart_t&& other)
    {
        while (!other.empty())
            push(other.remove());
    }

    // Concatenate other multipart to back
    void append(multipart_t&& other)
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
    void pushstr(const std::string& string)
    {
        m_parts.push_front(message_t(string.data(), string.size()));
    }

    // Push string to back
    void addstr(const std::string& string)
    {
        m_parts.push_back(message_t(string.data(), string.size()));
    }

    // Push type (fixed-size) to front
    template<typename T>
    void pushtyp(const T& type)
    {
        static_assert(!std::is_same<T, std::string>::value, "Use pushstr() instead of pushtyp<std::string>()");
        m_parts.push_front(message_t(&type, sizeof(type)));
    }

    // Push type (fixed-size) to back
    template<typename T>
    void addtyp(const T& type)
    {
        static_assert(!std::is_same<T, std::string>::value, "Use addstr() instead of addtyp<std::string>()");
        m_parts.push_back(message_t(&type, sizeof(type)));
    }

    // Push message part to front
    void push(message_t&& message)
    {
        m_parts.push_front(std::move(message));
    }

    // Push message part to back
    void add(message_t&& message)
    {
        m_parts.push_back(std::move(message));
    }

    // Pop string from front
    std::string popstr()
    {
        std::string string(m_parts.front().data<char>(), m_parts.front().size());
        m_parts.pop_front();
        return string;
    }

    // Pop type (fixed-size) from front
    template<typename T>
    T poptyp()
    {
        static_assert(!std::is_same<T, std::string>::value, "Use popstr() instead of poptyp<std::string>()");
        if (sizeof(T) != m_parts.front().size())
            throw std::runtime_error("Invalid type, size does not match the message size");
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
    const message_t* peek(size_t index) const
    {
        return &m_parts[index];
    }
    
    // Get a string copy of a specific message part
    std::string peekstr(size_t index) const
    {
        std::string string(m_parts[index].data<char>(), m_parts[index].size());
        return string;
    }

    // Peek type (fixed-size) from front
    template<typename T>
    T peektyp(size_t index) const
    {
        static_assert(!std::is_same<T, std::string>::value, "Use peekstr() instead of peektyp<std::string>()");
        if(sizeof(T) != m_parts[index].size())
            throw std::runtime_error("Invalid type, size does not match the message size");
        T type = *m_parts[index].data<T>();
        return type;
    }

    // Create multipart from type (fixed-size)
    template<typename T>
    static multipart_t create(const T& type)
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
        for (size_t i = 0; i < m_parts.size(); i++)
        {
            const unsigned char* data = m_parts[i].data<unsigned char>();
            size_t size = m_parts[i].size();

            // Dump the message as text or binary
            bool isText = true;
            for (size_t j = 0; j < size; j++)
            {
                if (data[j] < 32 || data[j] > 127)
                {
                    isText = false;
                    break;
                }
            }
            ss << "\n[" << std::dec << std::setw(3) << std::setfill('0') << size << "] ";
            if (size >= 1000)
            {
                ss << "... (to big to print)";
                continue;
            }
            for (size_t j = 0; j < size; j++)
            {
                if (isText)
                    ss << static_cast<char>(data[j]);
                else
                    ss << std::hex << std::setw(2) << std::setfill('0') << static_cast<short>(data[j]);
            }
        }
        return ss.str();
    }

    // Check if equal to other multipart
    bool equal(const multipart_t* other) const
    {
        if (size() != other->size())
            return false;
        for (size_t i = 0; i < size(); i++)
            if (!peek(i)->equal(other->peek(i)))
                return false;
        return true;
    }

    // Self test
    static int test()
    {
        bool ok = true; (void) ok;
        float num = 0; (void) num;
        std::string str = "";
        message_t msg;

        // Create two PAIR sockets and connect over inproc
        context_t context(1);
        socket_t output(context, ZMQ_PAIR);
        socket_t input(context, ZMQ_PAIR);
        output.bind("inproc://multipart.test");
        input.connect("inproc://multipart.test");

        // Test send and receive of single-frame message
        multipart_t multipart;
        assert(multipart.empty());

        multipart.push(message_t("Hello", 5));
        assert(multipart.size() == 1);

        ok = multipart.send(output);
        assert(multipart.empty());
        assert(ok);

        ok = multipart.recv(input);
        assert(multipart.size() == 1);
        assert(ok);

        msg = multipart.pop();
        assert(multipart.empty());
        assert(std::string(msg.data<char>(), msg.size()) == "Hello");

        // Test send and receive of multi-frame message
        multipart.addstr("A");
        multipart.addstr("BB");
        multipart.addstr("CCC");
        assert(multipart.size() == 3);

        multipart_t copy = multipart.clone();
        assert(copy.size() == 3);

        ok = copy.send(output);
        assert(copy.empty());
        assert(ok);

        ok = copy.recv(input);
        assert(copy.size() == 3);
        assert(ok);
        assert(copy.equal(&multipart));

        multipart.clear();
        assert(multipart.empty());

        // Test message frame manipulation
        multipart.add(message_t("Frame5", 6));
        multipart.addstr("Frame6");
        multipart.addstr("Frame7");
        multipart.addtyp(8.0f);
        multipart.addmem("Frame9", 6);
        multipart.push(message_t("Frame4", 6));
        multipart.pushstr("Frame3");
        multipart.pushstr("Frame2");
        multipart.pushtyp(1.0f);
        multipart.pushmem("Frame0", 6);
        assert(multipart.size() == 10);

        msg = multipart.remove();
        assert(multipart.size() == 9);
        assert(std::string(msg.data<char>(), msg.size()) == "Frame9");

        msg = multipart.pop();
        assert(multipart.size() == 8);
        assert(std::string(msg.data<char>(), msg.size()) == "Frame0");

        num = multipart.poptyp<float>();
        assert(multipart.size() == 7);
        assert(num == 1.0f);

        str = multipart.popstr();
        assert(multipart.size() == 6);
        assert(str == "Frame2");

        str = multipart.popstr();
        assert(multipart.size() == 5);
        assert(str == "Frame3");

        str = multipart.popstr();
        assert(multipart.size() == 4);
        assert(str == "Frame4");

        str = multipart.popstr();
        assert(multipart.size() == 3);
        assert(str == "Frame5");

        str = multipart.popstr();
        assert(multipart.size() == 2);
        assert(str == "Frame6");

        str = multipart.popstr();
        assert(multipart.size() == 1);
        assert(str == "Frame7");

        num = multipart.poptyp<float>();
        assert(multipart.empty());
        assert(num == 8.0f);

        // Test message constructors and concatenation
        multipart_t head("One", 3);
        head.addstr("Two");
        assert(head.size() == 2);

        multipart_t tail("One-hundred");
        tail.pushstr("Ninety-nine");
        assert(tail.size() == 2);

        multipart_t tmp(message_t("Fifty", 5));
        assert(tmp.size() == 1);

        multipart_t mid = multipart_t::create(49.0f);
        mid.append(std::move(tmp));
        assert(mid.size() == 2);
        assert(tmp.empty());

        multipart_t merged(std::move(mid));
        merged.prepend(std::move(head));
        merged.append(std::move(tail));
        assert(merged.size() == 6);
        assert(head.empty());
        assert(tail.empty());

        ok = merged.send(output);
        assert(merged.empty());
        assert(ok);

        multipart_t received(input);
        assert(received.size() == 6);

        str = received.popstr();
        assert(received.size() == 5);
        assert(str == "One");

        str = received.popstr();
        assert(received.size() == 4);
        assert(str == "Two");

        num = received.poptyp<float>();
        assert(received.size() == 3);
        assert(num == 49.0f);

        str = received.popstr();
        assert(received.size() == 2);
        assert(str == "Fifty");

        str = received.popstr();
        assert(received.size() == 1);
        assert(str == "Ninety-nine");

        str = received.popstr();
        assert(received.empty());
        assert(str == "One-hundred");

        return 0;
    }

private:
    // Disable implicit copying (moving is more efficient)
    multipart_t(const multipart_t& other) ZMQ_DELETED_FUNCTION;
    void operator=(const multipart_t& other) ZMQ_DELETED_FUNCTION;
};

#endif

}

#endif
