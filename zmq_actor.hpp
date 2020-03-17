/*
    Copyright (c) 2020 ZeroMQ community
    Copyright (c) 2020 Brett Viren

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

#ifndef __ZMQ_ACTOR_HPP_INCLUDED__
#define __ZMQ_ACTOR_HPP_INCLUDED__

#include "zmq.hpp"

#ifdef ZMQ_CPP11

#include <thread>

namespace zmq
{
/*! Return two paired PAIR sockets.
   
  First is bound and second is connected via inproc://.
*/
std::pair<zmq::socket_t, zmq::socket_t> create_linked_pairs(context_t &ctx)
{
    std::pair<zmq::socket_t, zmq::socket_t> ret{socket_t(ctx, socket_type::pair),
                                                socket_t(ctx, socket_type::pair)};

    std::stringstream ss;
    ss << "inproc://paired-" << std::hex << ret.first.handle() << "-"
       << ret.second.handle();
    std::string addr = ss.str();
    ret.first.bind(addr.c_str());
    ret.second.connect(addr.c_str());
    return ret;
}

/*! Spawn a function in a thread and communicate over a link.
     
  The actor function must take a socket and zero or more optional
  arguments such as:
     
      void func(zmq::socket sock, ...);
      zmq::actor_t actor(ctx, func, ...);
     
  The socket passed in is one end of a bidirection link shared
  with the application thread.  The application thread may get
  the other end of the link by calling zmq::actor_t::link().
     
  Note: unlike CZMQ zactor_t, zmq::actor_t does not institute any
  startup/shutdown protocol across the link.  If any is needed it
  is left to the calling application.
*/
class actor_t
{
  public:
    // Template constructor and not class to perform type erasure
    // of the function type.
    template<typename Func, typename... Args>
    actor_t(context_t &ctx, Func fn, Args... args)
    {
        socket_t asock;
        std::tie(asock, _sock) = create_linked_pairs(ctx);

        _thread = std::thread(
          [fn = std::forward<Func>(fn)](zmq::socket_t asock, Args... args) {
              fn(std::move(asock), std::forward<Args>(args)...);
          },
          std::move(asock), std::forward<Args>(args)...);
    }

    // Waits for actor function to exit.
    ~actor_t() { _thread.join(); }

    socket_ref link() { return _sock; }

  private:
    socket_t _sock;
    std::thread _thread;
};
}

#endif
#endif
