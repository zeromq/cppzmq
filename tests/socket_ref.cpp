#include <catch.hpp>
#include <zmq.hpp>
#ifdef ZMQ_CPP11

#ifdef ZMQ_CPP17
static_assert(std::is_nothrow_swappable_v<zmq::socket_ref>);
#endif
static_assert(sizeof(zmq::socket_ref) == sizeof(void *), "size mismatch");
static_assert(alignof(zmq::socket_ref) == alignof(void *), "alignment mismatch");
static_assert(ZMQ_IS_TRIVIALLY_COPYABLE(zmq::socket_ref),
    "needs to be trivially copyable");

TEST_CASE("socket_ref default init", "[socket_ref]")
{
    zmq::socket_ref sr;
    CHECK(!sr);
    CHECK(sr == nullptr);
    CHECK(nullptr == sr);
    CHECK(sr.handle() == nullptr);
}

TEST_CASE("socket_ref create from nullptr", "[socket_ref]")
{
    zmq::socket_ref sr = nullptr;
    CHECK(sr == nullptr);
    CHECK(sr.handle() == nullptr);
}

TEST_CASE("socket_ref create from handle", "[socket_ref]")
{
    void *np = nullptr;
    zmq::socket_ref sr{zmq::from_handle, np};
    CHECK(sr == nullptr);
    CHECK(sr.handle() == nullptr);
}

TEST_CASE("socket_ref compare", "[socket_ref]")
{
    zmq::socket_ref sr1;
    zmq::socket_ref sr2;
    CHECK(sr1 == sr2);
    CHECK(!(sr1 != sr2));
}

TEST_CASE("socket_ref compare from socket_t", "[socket_ref]")
{
    zmq::context_t context;
    zmq::socket_t s1(context, zmq::socket_type::router);
    zmq::socket_t s2(context, zmq::socket_type::dealer);
    zmq::socket_ref sr1 = s1;
    zmq::socket_ref sr2 = s2;
    CHECK(sr1);
    CHECK(sr2);
    CHECK(sr1 == s1);
    CHECK(sr2 == s2);
    CHECK(sr1.handle() == s1.handle());
    CHECK(sr1 != sr2);
    CHECK(sr1.handle() != sr2.handle());
    CHECK(sr1 != nullptr);
    CHECK(nullptr != sr1);
    CHECK(sr2 != nullptr);
    const bool comp1 = (sr1 < sr2) != (sr1 >= sr2);
    CHECK(comp1);
    const bool comp2 = (sr1 > sr2) != (sr1 <= sr2);
    CHECK(comp2);
    std::hash<zmq::socket_ref> hash;
    CHECK(hash(sr1) != hash(sr2));
    CHECK(hash(sr1) == hash(s1));
}

TEST_CASE("socket_ref assignment", "[socket_ref]")
{
    zmq::context_t context;
    zmq::socket_t s1(context, zmq::socket_type::router);
    zmq::socket_t s2(context, zmq::socket_type::dealer);
    zmq::socket_ref sr1 = s1;
    zmq::socket_ref sr2 = s2;
    sr1 = s2;
    CHECK(sr1 == sr2);
    CHECK(sr1.handle() == sr2.handle());
    sr1 = std::move(sr2);
    CHECK(sr1 == sr2);
    CHECK(sr1.handle() == sr2.handle());
    sr2 = nullptr;
    CHECK(sr1 != sr2);
    sr1 = nullptr;
    CHECK(sr1 == sr2);
}

TEST_CASE("socket_ref swap", "[socket_ref]")
{
    zmq::socket_ref sr1;
    zmq::socket_ref sr2;
    using std::swap;
    swap(sr1, sr2);
}

TEST_CASE("socket_ref type punning", "[socket_ref]")
{
    struct SVP
    {
        void *p;
    } svp;
    struct SSR
    {
        zmq::socket_ref sr;
    } ssr;

    zmq::context_t context;
    zmq::socket_t socket(context, zmq::socket_type::router);
    CHECK(socket.handle() != nullptr);
    svp.p = socket.handle();
    // static_cast to silence incorrect warning
    std::memcpy(static_cast<void *>(&ssr), &svp, sizeof(ssr));
    CHECK(ssr.sr == socket);
}

#endif
