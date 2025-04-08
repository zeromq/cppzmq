#include "common.hpp"
#include "corral/Nursery.h"
#include "corral/wait.h"
#include "zmq.hpp"
#include "zmq_addon.hpp"
#include <catch2/catch_test_macros.hpp>
#include <utility>

using namespace std::string_literals;
using namespace std::string_view_literals;
using namespace std::chrono_literals;
using zmq::async::corral::socket_t, zmq::message_t, zmq::context_t;


TEST_CASE("basic REQ and REP", "[async_corral]")
{
    boost::asio::io_context io;
    context_t ctx;

    constexpr auto req_msg = "Hi"sv;
    constexpr auto rep_msg = "There"sv;
    constexpr auto inproc_addr = "inproc://async_corral-basic";

    corral::run(io, [&] -> corral::Task<> {
        co_await corral::allOf(
          [&] -> corral::Task<> {
              socket_t socket{io, ctx, zmq::socket_type::req};
              socket.connect(inproc_addr);
              co_await socket.send(message_t{req_msg});
              auto msg = co_await socket.recv();
              REQUIRE(msg.to_string() == rep_msg);
          },
          [&] -> corral::Task<> {
              socket_t socket{io, ctx, zmq::socket_type::rep};
              socket.bind(inproc_addr);
              auto r = co_await socket.recv();
              REQUIRE(r.to_string() == req_msg);
              co_await socket.send(message_t{rep_msg});
          });
    });
}

TEST_CASE("simple ROUTER and DEALER", "[async_corral]")
{
    boost::asio::io_context io;
    context_t ctx;

    constexpr auto request_msg1 = "Test"sv;
    constexpr auto request_msg2 = "ing"sv;
    constexpr auto response_msg = "42!"sv;
    constexpr auto response_repeat = 2;
    constexpr auto inproc_addr = "inproc://async_corral-router_dealer";

    auto server = [&] -> corral::Task<> {
        auto external = socket_t{io, ctx, zmq::socket_type::router};
        external.bind(inproc_addr);

        for (;;) {
            auto msg = co_await external.recv_multipart();
            REQUIRE(msg.size() == 3);
            REQUIRE(msg[1].to_string_view() == request_msg1);
            REQUIRE(msg[2].to_string_view() == request_msg2);
            auto routing_id = msg.pop();

            for (auto i = 0; i < response_repeat; ++i) {
                zmq::multipart_t response;
                response.add(std::move(message_t{routing_id.to_string_view()}));
                response.add(message_t{response_msg});
                co_await external.send(std::move(response));
                co_await Timer{5ms, io};
            }
        }
    };


    auto client = [&] -> corral::Task<> {
        auto socket = socket_t{io.get_executor(), ctx, zmq::socket_type::dealer};
        socket.connect(inproc_addr);

        for (auto i = 0; i < 3; ++i) {
            zmq::multipart_t msg;
            msg.add(message_t{request_msg1});
            msg.add(message_t{request_msg2});
            co_await socket.send(std::move(msg));

            for (auto i = 0; i < response_repeat; ++i) {
                auto response = co_await socket.recv_multipart();
                REQUIRE(response.size() == 1);
                REQUIRE(response[0].to_string_view() == response_msg);
            }
        }
    };

    corral::run(io, corral::anyOf(client(), server()));
}

TEST_CASE("ROUTER forwarding", "[async_corral]")
{
    // dealer client -> external router
    // external router -> work dispatcher (spawn a new worker)
    // worker -> internal router
    // (forward) internal router -> external router


    boost::asio::io_context io;
    context_t ctx;

    constexpr auto request_msg1 = "Test"sv;
    constexpr auto request_msg2 = "ing"sv;
    constexpr auto response_msg = "42!"sv;
    constexpr auto response_repeat = 2;
    constexpr auto inproc_external_addr =
      "inproc://async_corral-router_forwarding-router";
    constexpr auto inproc_internal_addr =
      "inproc://async_corral-router_forwarding-rep";

    auto worker = [&](socket_t dealer, zmq::multipart_t msg) -> corral::Task<> {
        REQUIRE(msg.size() == 2);
        REQUIRE(msg[0].to_string_view() == request_msg1);
        REQUIRE(msg[1].to_string_view() == request_msg2);
        for (auto i = 0; i < response_repeat; ++i) {
            co_await dealer.send(message_t{response_msg});
            co_await Timer{50ms, io};
        }
    };

    auto work_dispatcher = [&](socket_t &external) -> corral::Task<> {
        CORRAL_WITH_NURSERY(n)
        {
            for (;;) {
                auto msg = co_await external.recv_multipart();

                auto worker_socket = socket_t{io, ctx, zmq::socket_type::dealer};
                worker_socket.set(zmq::sockopt::routing_id, msg[0].to_string_view());
                worker_socket.connect(inproc_internal_addr);
                msg.pop();
                n.start(worker, std::move(worker_socket), std::move(msg));
            }
        };
    };

    auto forward = [&](socket_t &external, socket_t &internal) -> corral::Task<> {
        for (;;) {
            auto msg_from_internal = co_await internal.recv_multipart();
            co_await external.send(std::move(msg_from_internal));
        }
    };

    auto server = [&] -> corral::Task<> {
        auto external = socket_t{io, ctx, zmq::socket_type::router};
        auto internal = socket_t{io, ctx, zmq::socket_type::router};

        external.bind(inproc_external_addr);
        internal.bind(inproc_internal_addr);

        co_await corral::anyOf(forward(external, internal),
                               work_dispatcher(external));
    };

    auto client = [&] -> corral::Task<> {
        auto socket = socket_t{io.get_executor(), ctx, zmq::socket_type::dealer};
        socket.connect(inproc_external_addr);

        zmq::multipart_t msg;
        msg.add(message_t{request_msg1});
        msg.add(message_t{request_msg2});
        co_await socket.send(std::move(msg));

        for (auto i = 0; i < response_repeat; ++i) {
            auto response = co_await socket.recv_multipart();
            REQUIRE(response.size() == 1);
            REQUIRE(response[0].to_string_view() == response_msg);
        }
    };

    corral::run(io, corral::anyOf(client(), server()));
}
