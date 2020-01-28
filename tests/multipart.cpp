#include <catch.hpp>
#include <zmq_addon.hpp>

#ifdef ZMQ_HAS_RVALUE_REFS
/// \todo split this up into separate test cases
///
TEST_CASE("multipart legacy test", "[multipart]")
{
    using namespace zmq;

    bool ok = true;
    (void) ok;
    float num = 0;
    (void) num;
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

    const message_t &front_msg = multipart.front();
    assert(multipart.size() == 10);
    assert(std::string(front_msg.data<char>(), front_msg.size()) == "Frame0");

    const message_t &back_msg = multipart.back();
    assert(multipart.size() == 10);
    assert(std::string(back_msg.data<char>(), back_msg.size()) == "Frame9");

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

    multipart_t tail(std::string("One-hundred"));
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
}
#endif
