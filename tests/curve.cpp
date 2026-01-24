#include <catch2/catch_all.hpp>
#include <zmq.hpp>

#ifdef ZMQ_HAVE_CURVE

TEST_CASE("curve_keypair", "[curve]")
{
    auto keys = zmq::curve_keypair();
    auto public_key = keys.first;
    auto secret_key = keys.second;
    CHECK(!public_key.empty());
    CHECK(!secret_key.empty());
}

TEST_CASE("curve_public", "[curve]")
{
    auto secret_key = "D:)Q[IlAW!ahhC2ac:9*A}h:p?([4%wOTJ%JR%cs";
    auto public_key = zmq::curve_public(secret_key);
    CHECK(public_key == "Yne@$w-vo<fVvi]a<NY6T1ed:M$fCG*[IaLV{hID");
}

#endif

TEST_CASE("z85_encode", "[curve]")
{
    std::vector<uint8_t> data{1, 2, 3, 4, 5, 6, 7, 8};
    auto encoded = zmq::z85_encode(data);
    CHECK(encoded.size() == std::string("0rJua1Qkhq").size());
    CHECK(encoded == "0rJua1Qkhq");
}

TEST_CASE("z85_decode", "[curve]")
{
    auto decoded = zmq::z85_decode("0rJua1Qkhq");
    CHECK(decoded == std::vector<uint8_t>{1, 2, 3, 4, 5, 6, 7, 8});
}
