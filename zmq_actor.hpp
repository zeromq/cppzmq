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
// Actor needs some threading.
// Initial implementation uses std::thread so C++11 only.
#include <thread>

namespace zmq {

    typedef std::pair<zmq::socket_t, zmq::socket_t> pipe;

    /*! Return a pair of PAIR sockets.
     *
     * First is bound and second is connected via inproc://
     */
    pipe create_pipe(context_t& ctx)
    {
        pipe ret{socket_t(ctx, socket_type::pair),
                 socket_t(ctx, socket_type::pair)};

        std::stringstream ss;
        ss << "inproc://pipe-"
           << (void*) ret.first.handle()
           << "-"
           << (void*) ret.second.handle();
        std::string addr = ss.str();
        ret.first.bind(addr.c_str());
        ret.second.connect(addr.c_str());
        return ret;
    }


    // This shim calls the actor function and then provides hard-wired
    // protocol for shutdown, as per CZMQ zactor_t.
    // 
    // fixme: can't figure out how to forward the func nor the args so
    // we must pass by value only.
    template<typename Func, typename... Args>
    void actor_shim(socket_t&& apipe, Func fn, Args... args) {
        fn(apipe, args...);
        apipe.send(zmq::message_t{}, zmq::send_flags::none);
    }

    /*! An CZMQ zactor_t like class.
     *
     * Construct it with an actor function and any args.  
     *
     * The function will be spawned in a thread.
     *
     * Application may use pipe() to communicate with the actor.
     *
     * The desctructor will notify and wait for the actor function to
     * exit, if it is still around.
     */
    class actor_t {
    public:                     // for now for testing
        socket_t _pipe;
        std::thread _thread;
    public:

        template<typename Func, typename... Args>
        actor_t(context_t& ctx, Func fn, Args... args) {
            socket_t apipe;
            std::tie(apipe, _pipe) = create_pipe(ctx);
                              
            _thread = std::thread(actor_shim<Func, Args...>,
                                  std::move(apipe),
                                  fn,
                                  args...);

            zmq::message_t rmsg; // wait for actor ready signal
            auto res = _pipe.recv(rmsg);
        }

        ~actor_t() {
            auto sres = _pipe.send(message_t("$TERM",5), send_flags::dontwait);
            if (sres) {
                message_t rmsg;
                auto res = _pipe.recv(rmsg, recv_flags::none);
            }
            _thread.join();
        }

        socket_t& pipe() { return _pipe; }
        
    private:
        actor_t(const actor_t &) ZMQ_DELETED_FUNCTION;
        void operator=(const actor_t &) ZMQ_DELETED_FUNCTION;

    };
    
}

#endif
#endif
