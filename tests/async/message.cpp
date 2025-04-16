#include "common.hpp"
#include <exec/async_scope.hpp>
#include "zmq.hpp"
#include "zmq_async.hpp"
#include <exec/task.hpp>
#include <boost/asio/thread_pool.hpp>
#include <catch2/catch_test_macros.hpp>
#include <utility>

using namespace std::string_literals;
using namespace std::string_view_literals;
using namespace std::chrono_literals;
namespace ex = stdexec;
using zmq::async::v1::async_socket_t, zmq::async::v1::recv_multipart,
  zmq::async::v1::send_multipart, zmq::message_t, zmq::context_t, zmq::socket_type;


TEST_CASE("basic REQ and REP", "[async_stdexec]")
{
    boost::asio::thread_pool io;
    context_t ctx;

    constexpr auto req_msg = "Hi"sv;
    constexpr auto rep_msg = "There"sv;
    constexpr auto inproc_addr = "inproc://async_stdexec-basic";

    ex::sync_wait(ex::when_all(
      [&] -> exec::task<void> {
          async_socket_t socket{io.get_executor(), ctx, zmq::socket_type::req};
          socket.connect(inproc_addr);
          co_await socket.send(message_t{req_msg});
          auto msg = co_await socket.recv();
          REQUIRE(msg.to_string() == rep_msg);
      }(),
      [&] -> exec::task<void> {
          async_socket_t socket{io.get_executor(), ctx, zmq::socket_type::rep};
          socket.bind(inproc_addr);
          auto r = co_await socket.recv();
          REQUIRE(r.to_string() == req_msg);
          co_await socket.send(message_t{rep_msg});
      }()));
}

#if 1
TEST_CASE("simple ROUTER and DEALER", "[async_stdexec]")
{
    boost::asio::thread_pool io;
    context_t ctx;

    constexpr auto request_msg1 = "Test"sv;
    constexpr auto request_msg2 = "ing"sv;
    constexpr auto response_msg = "42!"sv;
    constexpr auto response_repeat = 2;
    constexpr auto inproc_addr = "inproc://async_stdexec-router_dealer";

    auto server = [&] -> exec::task<void> {
        auto external =
          async_socket_t{io.get_executor(), ctx, zmq::socket_type::router};
        external.bind(inproc_addr);

        for (;;) {
            auto msg = co_await recv_multipart(external);
            REQUIRE(msg.size() == 3);
            REQUIRE(msg[1].to_string_view() == request_msg1);
            REQUIRE(msg[2].to_string_view() == request_msg2);
            auto routing_id = msg.pop();

            for (auto i = 0; i < response_repeat; ++i) {
                zmq::multipart_t response;
                response.add(std::move(message_t{routing_id.to_string_view()}));
                response.add(message_t{response_msg});
                co_await send_multipart(external, std::move(response));
                // co_await Timer{5ms, io.get_executor()};
            }
        }
    };


    auto client = [&] -> exec::task<void> {
        auto socket =
          async_socket_t{io.get_executor(), ctx, zmq::socket_type::dealer};
        socket.connect(inproc_addr);

        for (auto i = 0; i < 3; ++i) {
            zmq::multipart_t msg;
            msg.add(message_t{request_msg1});
            msg.add(message_t{request_msg2});
            co_await send_multipart(socket, std::move(msg));

            for (auto i = 0; i < response_repeat; ++i) {
                auto response = co_await recv_multipart(socket);
                REQUIRE(response.size() == 1);
                REQUIRE(response[0].to_string_view() == response_msg);
            }
        }
    };

    ex::sync_wait(exec::when_any(client(), server()));
}
#endif

#if 1
TEST_CASE("ROUTER forwarding", "[async_stdexec]")
{
    // dealer client -> external router
    // external router -> work dispatcher (spawn a new worker)
    // worker -> internal router
    // (forward) internal router -> external router


    boost::asio::thread_pool io;
    context_t ctx;

    constexpr auto request_msg1 = "Test"sv;
    constexpr auto request_msg2 = "ing"sv;
    constexpr auto response_msg = "42!"sv;
    constexpr auto response_repeat = 2;
    constexpr auto inproc_external_addr =
      "inproc://async_stdexec-router_forwarding-router";
    constexpr auto inproc_internal_addr =
      "inproc://async_stdexec-router_forwarding-rep";

    auto worker = [&](async_socket_t<socket_type::dealer> dealer,
                      zmq::multipart_t msg) -> exec::task<void> {
        REQUIRE(msg.size() == 2);
        REQUIRE(msg[0].to_string_view() == request_msg1);
        REQUIRE(msg[1].to_string_view() == request_msg2);
        for (auto i = 0; i < response_repeat; ++i) {
            co_await dealer.send(message_t{response_msg});
            // co_await Timer{50ms, io.get_executor()};
        }
    };

    auto work_dispatcher =
      [&](async_socket_t<socket_type::router> &external) -> exec::task<void> {
        exec::async_scope scope;
        for (;;) {
            auto msg = co_await recv_multipart(external);

            auto worker_socket =
              async_socket_t<zmq::socket_type::dealer>{io.get_executor(), ctx};
            worker_socket.set(zmq::sockopt::routing_id, msg[0].to_string_view());
            worker_socket.connect(inproc_internal_addr);
            msg.pop();
            scope.spawn(worker(std::move(worker_socket), std::move(msg)));
        };
    };

    auto forward =
      [&](async_socket_t<socket_type::router> &external,
          async_socket_t<socket_type::router> &internal) -> exec::task<void> {
        for (;;) {
            auto msg_from_internal = co_await recv_multipart(internal);
            co_await send_multipart(external, std::move(msg_from_internal));
        }
    };

    auto server = [&] -> exec::task<void> {
        auto external = async_socket_t<socket_type::router>{io.get_executor(), ctx};
        auto internal = async_socket_t<socket_type::router>{io.get_executor(), ctx};

        external.bind(inproc_external_addr);
        internal.bind(inproc_internal_addr);

        co_await exec::when_any(forward(external, internal),
                                work_dispatcher(external));
    };

    auto client = [&] -> exec::task<void> {
        auto socket = async_socket_t<socket_type::dealer>{io.get_executor(), ctx};
        socket.connect(inproc_external_addr);

        zmq::multipart_t msg;
        msg.add(message_t{request_msg1});
        msg.add(message_t{request_msg2});
        co_await send_multipart(socket, std::move(msg));

        for (auto i = 0; i < response_repeat; ++i) {
            auto response = co_await recv_multipart(socket);
            REQUIRE(response.size() == 1);
            REQUIRE(response[0].to_string_view() == response_msg);
        }
    };

    ex::sync_wait(exec::when_any(client(), server()));
}
#endif


TEST_CASE("ROUTER proxy", "[async_stdexec]")
{
    constexpr auto inproc_frontend_addr =
      "inproc://async_stdexec-router_proxy-frontend";
    constexpr auto inproc_backend_addr =
      "inproc://async_stdexec-router_proxy-backend";


    boost::asio::thread_pool io;
    context_t ctx;

    async_socket_t<socket_type::router> frontend{io.get_executor(), ctx};
    frontend.bind(inproc_frontend_addr);
    async_socket_t<socket_type::dealer> backend{io.get_executor(), ctx};
    backend.bind(inproc_backend_addr);

    auto client_process = [&io, &ctx] -> exec::task<void> {
        async_socket_t<socket_type::req> client{io.get_executor(), ctx};
        client.connect(inproc_frontend_addr);

        for (auto i = 0; i < 3; ++i) {
            co_await client.send(zmq::message_t{"Hi!"s});
            auto response = co_await client.recv();
            REQUIRE(response.to_string_view() == "Worker!");
        }
    };

    auto worker_process = [&] -> exec::task<void> {
        for (;;) {
            async_socket_t<socket_type::dealer> worker{io.get_executor(), ctx};
            worker.connect(inproc_backend_addr);

            auto msg = co_await recv_multipart(worker);
            auto routing_id = msg.pop();
            auto _ = msg.pop();
            auto hello = msg.pop();
            REQUIRE(hello.to_string_view() == "Hi!");
            auto response = std::array{std::move(routing_id), std::move(_),
                                       zmq::message_t{"Worker!"s}};
            co_await send_multipart(worker, response);
        }
    };

    ex::sync_wait(exec::when_any(
      client_process(), worker_process(),
      zmq::async::v1::proxy(std::move(frontend), std::move(backend))));
}