#include <catch.hpp>
#include <zmq_addon.hpp>

#ifdef ZMQ_CPP11

TEST_CASE("multipart codec empty", "[codec_multipart]")
{
    using namespace zmq;

    multipart_t mmsg;
    message_t msg = mmsg.encode();
    CHECK(msg.size() == 0);
    
    multipart_t mmsg2;
    mmsg2.decode_append(msg);
    CHECK(mmsg2.size() == 0);

}

TEST_CASE("multipart codec small", "[codec_multipart]")
{
    using namespace zmq;

    multipart_t mmsg;
    mmsg.addstr("Hello World");
    message_t msg = mmsg.encode();
    CHECK(msg.size() == 1 + 11); // small size packing

    mmsg.addstr("Second frame");
    msg = mmsg.encode();
    CHECK(msg.size() == 1 + 11 + 1 + 12);

    multipart_t mmsg2;
    mmsg2.decode_append(msg);
    CHECK(mmsg2.size() == 2);
    std::string part0 = mmsg2[0].to_string();
    CHECK(part0 == "Hello World");
    CHECK(mmsg2[1].to_string() == "Second frame");
}

TEST_CASE("multipart codec big", "[codec_multipart]")
{
    using namespace zmq;

    message_t big(495);         // large size packing
    big.data<char>()[0] = 'X';

    multipart_t mmsg;
    mmsg.pushmem(big.data(), big.size());
    message_t msg = mmsg.encode();
    CHECK(msg.size() == 5 + 495);
    CHECK(msg.data<unsigned char>()[0] == std::numeric_limits<uint8_t>::max());
    CHECK(msg.data<unsigned char>()[5] == 'X');

    CHECK(mmsg.size() == 1);
    mmsg.decode_append(msg);
    CHECK(mmsg.size() == 2);
    CHECK(mmsg[0].data<char>()[0] == 'X');
}

TEST_CASE("multipart codec decode bad data overflow", "[codec_multipart]")
{
    using namespace zmq;

    char bad_data[3] = {5, 'h', 'i'};
    message_t wrong_size(bad_data, 3);
    CHECK(wrong_size.size() == 3);
    CHECK(wrong_size.data<char>()[0] == 5);
    
    CHECK_THROWS_AS(
        multipart_t::decode(wrong_size),
        const std::out_of_range&);
}

TEST_CASE("multipart codec decode bad data extra data", "[codec_multipart]")
{
    using namespace zmq;

    char bad_data[3] = {1, 'h', 'i'};
    message_t wrong_size(bad_data, 3);
    CHECK(wrong_size.size() == 3);
    CHECK(wrong_size.data<char>()[0] == 1);
    
    CHECK_THROWS_AS(
        multipart_t::decode(wrong_size),
        const std::out_of_range&);
}


// After exercising it, this test is disabled over concern of running
// on hosts which lack enough free memory to allow the absurdly large
// message part to be allocated.
#if 0
TEST_CASE("multipart codec encode too big", "[codec_multipart]")
{
    using namespace zmq;

    const size_t too_big_size = 1L + std::numeric_limits<uint32_t>::max();
    CHECK(too_big_size > std::numeric_limits<uint32_t>::max());
    char* too_big_data = new char[too_big_size];
    multipart_t mmsg(too_big_data, too_big_size);
    delete [] too_big_data;

    CHECK(mmsg.size() == 1);
    CHECK(mmsg[0].size() > std::numeric_limits<uint32_t>::max());

    CHECK_THROWS_AS(
        mmsg.encode(),
        std::range_error);
}
#endif

TEST_CASE("multipart codec free function with vector of message_t", "[codec_multipart]")
{
    using namespace zmq;
    std::vector<message_t> parts;
    parts.emplace_back("Hello", 5);
    parts.emplace_back("World",5);
    auto msg = encode(parts);
    CHECK(msg.size() == 1 + 5 + 1 + 5 );
    CHECK(msg.data<unsigned char>()[0] == 5);
    CHECK(msg.data<unsigned char>()[1] == 'H');
    CHECK(msg.data<unsigned char>()[6] == 5);
    CHECK(msg.data<unsigned char>()[7] == 'W');

    std::vector<message_t> parts2;
    decode(msg, std::back_inserter(parts2));
    CHECK(parts.size() == 2);
    CHECK(parts[0].size() == 5);
    CHECK(parts[1].size() == 5);
}

TEST_CASE("multipart codec free function with vector of const_buffer", "[codec_multipart]")
{
    using namespace zmq;
    std::vector<const_buffer> parts;
    parts.emplace_back("Hello", 5);
    parts.emplace_back("World",5);
    auto msg = encode(parts);
    CHECK(msg.size() == 1 + 5 + 1 + 5 );
    CHECK(msg.data<unsigned char>()[0] == 5);
    CHECK(msg.data<unsigned char>()[1] == 'H');
    CHECK(msg.data<unsigned char>()[6] == 5);
    CHECK(msg.data<unsigned char>()[7] == 'W');

    std::vector<message_t> parts2;
    decode(msg, std::back_inserter(parts2));
    CHECK(parts.size() == 2);
    CHECK(parts[0].size() == 5);
    CHECK(parts[1].size() == 5);
}

TEST_CASE("multipart codec free function with vector of mutable_buffer", "[codec_multipart]")
{
    using namespace zmq;
    std::vector<mutable_buffer> parts;
    char hello[6] = "Hello";
    parts.emplace_back(hello, 5);
    char world[6] = "World";
    parts.emplace_back(world,5);
    auto msg = encode(parts);
    CHECK(msg.size() == 1 + 5 + 1 + 5 );
    CHECK(msg.data<unsigned char>()[0] == 5);
    CHECK(msg.data<unsigned char>()[1] == 'H');
    CHECK(msg.data<unsigned char>()[6] == 5);
    CHECK(msg.data<unsigned char>()[7] == 'W');

    std::vector<message_t> parts2;
    decode(msg, std::back_inserter(parts2));
    CHECK(parts.size() == 2);
    CHECK(parts[0].size() == 5);
    CHECK(parts[1].size() == 5);
}

TEST_CASE("multipart codec free function with multipart_t", "[codec_multipart]")
{
    using namespace zmq;
    multipart_t mmsg;
    mmsg.addstr("Hello");
    mmsg.addstr("World");
    auto msg = encode(mmsg);
    CHECK(msg.size() == 1 + 5 + 1 + 5);
    CHECK(msg.data<unsigned char>()[0] == 5);
    CHECK(msg.data<unsigned char>()[1] == 'H');
    CHECK(msg.data<unsigned char>()[6] == 5);
    CHECK(msg.data<unsigned char>()[7] == 'W');

    multipart_t mmsg2;
    decode(msg, std::back_inserter(mmsg2));
    CHECK(mmsg2.size() == 2);
    CHECK(mmsg2[0].size() == 5);
    CHECK(mmsg2[1].size() == 5);
}

TEST_CASE("multipart codec static method decode to multipart_t", "[codec_multipart]")
{
    using namespace zmq;
    multipart_t mmsg;
    mmsg.addstr("Hello");
    mmsg.addstr("World");
    auto msg = encode(mmsg);

    auto mmsg2 = multipart_t::decode(msg);
    CHECK(mmsg2.size() == 2);
    CHECK(mmsg2[0].size() == 5);
    CHECK(mmsg2[1].size() == 5);
}


#endif
