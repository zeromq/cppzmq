#include <iostream>
#include <zmq_addon.hpp>

int main()
{
    zmq::context_t ctx;
    zmq::socket_t sock1(ctx, zmq::socket_type::push);
    zmq::socket_t sock2(ctx, zmq::socket_type::pull);
    sock1.bind("tcp://127.0.0.1:*");
    const std::string last_endpoint =
        sock1.get(zmq::sockopt::last_endpoint);
    std::cout << "Connecting to "
              << last_endpoint << std::endl;
    sock2.connect(last_endpoint);

    std::array<zmq::const_buffer, 2> send_msgs = {
        zmq::str_buffer("foo"),
        zmq::str_buffer("bar!")
    };
    if (!zmq::send_multipart(sock1, send_msgs))
        return 1;

    std::vector<zmq::message_t> recv_msgs;
    const auto ret = zmq::recv_multipart(
        sock2, std::back_inserter(recv_msgs));
    if (!ret)
        return 1;
    std::cout << "Got " << *ret
              << " messages" << std::endl;
    return 0;
}
