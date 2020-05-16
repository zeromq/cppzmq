#include <catch.hpp>
#include <zmq.hpp>

#ifdef ZMQ_CPP17
static_assert(std::is_nothrow_swappable_v<zmq::const_buffer>);
static_assert(std::is_nothrow_swappable_v<zmq::mutable_buffer>);
static_assert(std::is_trivially_copyable_v<zmq::const_buffer>);
static_assert(std::is_trivially_copyable_v<zmq::mutable_buffer>);
#endif

#ifdef ZMQ_CPP11

using BT = int16_t;

TEST_CASE("buffer default ctor", "[buffer]")
{
    constexpr zmq::mutable_buffer mb;
    constexpr zmq::const_buffer cb;
    CHECK(mb.size() == 0);
    CHECK(mb.data() == nullptr);
    CHECK(cb.size() == 0);
    CHECK(cb.data() == nullptr);
}

TEST_CASE("buffer data ctor", "[buffer]")
{
    std::vector<BT> v(10);
    zmq::const_buffer cb(v.data(), v.size() * sizeof(BT));
    CHECK(cb.size() == v.size() * sizeof(BT));
    CHECK(cb.data() == v.data());
    zmq::mutable_buffer mb(v.data(), v.size() * sizeof(BT));
    CHECK(mb.size() == v.size() * sizeof(BT));
    CHECK(mb.data() == v.data());
    zmq::const_buffer from_mut = mb;
    CHECK(mb.size() == from_mut.size());
    CHECK(mb.data() == from_mut.data());
    const auto cmb = mb;
    static_assert(std::is_same<decltype(cmb.data()), void *>::value, "");

    constexpr const void *cp = nullptr;
    constexpr void *p = nullptr;
    constexpr zmq::const_buffer cecb = zmq::buffer(p, 0);
    constexpr zmq::mutable_buffer cemb = zmq::buffer(p, 0);
    CHECK(cecb.data() == nullptr);
    CHECK(cemb.data() == nullptr);
}

TEST_CASE("const_buffer operator+", "[buffer]")
{
    std::vector<BT> v(10);
    zmq::const_buffer cb(v.data(), v.size() * sizeof(BT));
    const size_t shift = 4;
    auto shifted = cb + shift;
    CHECK(shifted.size() == v.size() * sizeof(BT) - shift);
    CHECK(shifted.data() == v.data() + shift / sizeof(BT));
    auto shifted2 = shift + cb;
    CHECK(shifted.size() == shifted2.size());
    CHECK(shifted.data() == shifted2.data());
    auto cbinp = cb;
    cbinp += shift;
    CHECK(shifted.size() == cbinp.size());
    CHECK(shifted.data() == cbinp.data());
}

TEST_CASE("mutable_buffer operator+", "[buffer]")
{
    std::vector<BT> v(10);
    zmq::mutable_buffer mb(v.data(), v.size() * sizeof(BT));
    const size_t shift = 4;
    auto shifted = mb + shift;
    CHECK(shifted.size() == v.size() * sizeof(BT) - shift);
    CHECK(shifted.data() == v.data() + shift / sizeof(BT));
    auto shifted2 = shift + mb;
    CHECK(shifted.size() == shifted2.size());
    CHECK(shifted.data() == shifted2.data());
    auto mbinp = mb;
    mbinp += shift;
    CHECK(shifted.size() == mbinp.size());
    CHECK(shifted.data() == mbinp.data());
}

TEST_CASE("mutable_buffer creation basic", "[buffer]")
{
    std::vector<BT> v(10);
    zmq::mutable_buffer mb(v.data(), v.size() * sizeof(BT));
    zmq::mutable_buffer mb2 = zmq::buffer(v.data(), v.size() * sizeof(BT));
    CHECK(mb.data() == mb2.data());
    CHECK(mb.size() == mb2.size());
    zmq::mutable_buffer mb3 = zmq::buffer(mb);
    CHECK(mb.data() == mb3.data());
    CHECK(mb.size() == mb3.size());
    zmq::mutable_buffer mb4 = zmq::buffer(mb, 10 * v.size() * sizeof(BT));
    CHECK(mb.data() == mb4.data());
    CHECK(mb.size() == mb4.size());
    zmq::mutable_buffer mb5 = zmq::buffer(mb, 4);
    CHECK(mb.data() == mb5.data());
    CHECK(4 == mb5.size());
}

TEST_CASE("const_buffer creation basic", "[buffer]")
{
    const std::vector<BT> v(10);
    zmq::const_buffer cb(v.data(), v.size() * sizeof(BT));
    zmq::const_buffer cb2 = zmq::buffer(v.data(), v.size() * sizeof(BT));
    CHECK(cb.data() == cb2.data());
    CHECK(cb.size() == cb2.size());
    zmq::const_buffer cb3 = zmq::buffer(cb);
    CHECK(cb.data() == cb3.data());
    CHECK(cb.size() == cb3.size());
    zmq::const_buffer cb4 = zmq::buffer(cb, 10 * v.size() * sizeof(BT));
    CHECK(cb.data() == cb4.data());
    CHECK(cb.size() == cb4.size());
    zmq::const_buffer cb5 = zmq::buffer(cb, 4);
    CHECK(cb.data() == cb5.data());
    CHECK(4 == cb5.size());
}

TEST_CASE("mutable_buffer creation C array", "[buffer]")
{
    BT d[10] = {};
    zmq::mutable_buffer b = zmq::buffer(d);
    CHECK(b.size() == 10 * sizeof(BT));
    CHECK(b.data() == static_cast<BT*>(d));
    zmq::const_buffer b2 = zmq::buffer(d, 4);
    CHECK(b2.size() == 4);
    CHECK(b2.data() == static_cast<BT*>(d));
}

TEST_CASE("const_buffer creation C array", "[buffer]")
{
    const BT d[10] = {};
    zmq::const_buffer b = zmq::buffer(d);
    CHECK(b.size() == 10 * sizeof(BT));
    CHECK(b.data() == static_cast<const BT*>(d));
    zmq::const_buffer b2 = zmq::buffer(d, 4);
    CHECK(b2.size() == 4);
    CHECK(b2.data() == static_cast<const BT*>(d));
}

TEST_CASE("mutable_buffer creation array", "[buffer]")
{
    std::array<BT, 10> d = {};
    zmq::mutable_buffer b = zmq::buffer(d);
    CHECK(b.size() == d.size() * sizeof(BT));
    CHECK(b.data() == d.data());
    zmq::mutable_buffer b2 = zmq::buffer(d, 4);
    CHECK(b2.size() == 4);
    CHECK(b2.data() == d.data());
}

TEST_CASE("const_buffer creation array", "[buffer]")
{
    const std::array<BT, 10> d = {};
    zmq::const_buffer b = zmq::buffer(d);
    CHECK(b.size() == d.size() * sizeof(BT));
    CHECK(b.data() == d.data());
    zmq::const_buffer b2 = zmq::buffer(d, 4);
    CHECK(b2.size() == 4);
    CHECK(b2.data() == d.data());
}

TEST_CASE("const_buffer creation array 2", "[buffer]")
{
    std::array<const BT, 10> d = {{}};
    zmq::const_buffer b = zmq::buffer(d);
    CHECK(b.size() == d.size() * sizeof(BT));
    CHECK(b.data() == d.data());
    zmq::const_buffer b2 = zmq::buffer(d, 4);
    CHECK(b2.size() == 4);
    CHECK(b2.data() == d.data());
}

TEST_CASE("mutable_buffer creation vector", "[buffer]")
{
    std::vector<BT> d(10);
    zmq::mutable_buffer b = zmq::buffer(d);
    CHECK(b.size() == d.size() * sizeof(BT));
    CHECK(b.data() == d.data());
    zmq::mutable_buffer b2 = zmq::buffer(d, 4);
    CHECK(b2.size() == 4);
    CHECK(b2.data() == d.data());
    d.clear();
    b = zmq::buffer(d);
    CHECK(b.size() == 0);
    CHECK(b.data() == nullptr);
}

TEST_CASE("const_buffer creation vector", "[buffer]")
{
    std::vector<BT> d(10);
    zmq::const_buffer b = zmq::buffer(static_cast<const std::vector<BT> &>(d));
    CHECK(b.size() == d.size() * sizeof(BT));
    CHECK(b.data() == d.data());
    zmq::const_buffer b2 = zmq::buffer(static_cast<const std::vector<BT> &>(d), 4);
    CHECK(b2.size() == 4);
    CHECK(b2.data() == d.data());
    d.clear();
    b = zmq::buffer(static_cast<const std::vector<BT> &>(d));
    CHECK(b.size() == 0);
    CHECK(b.data() == nullptr);
}

TEST_CASE("const_buffer creation string", "[buffer]")
{
    const std::wstring d(10, L'a');
    zmq::const_buffer b = zmq::buffer(d);
    CHECK(b.size() == d.size() * sizeof(wchar_t));
    CHECK(b.data() == d.data());
    zmq::const_buffer b2 = zmq::buffer(d, 4);
    CHECK(b2.size() == 4);
    CHECK(b2.data() == d.data());
}

TEST_CASE("mutable_buffer creation string", "[buffer]")
{
    std::wstring d(10, L'a');
    zmq::mutable_buffer b = zmq::buffer(d);
    CHECK(b.size() == d.size() * sizeof(wchar_t));
    CHECK(b.data() == d.data());
    zmq::mutable_buffer b2 = zmq::buffer(d, 4);
    CHECK(b2.size() == 4);
    CHECK(b2.data() == d.data());
}

#if CPPZMQ_HAS_STRING_VIEW
TEST_CASE("const_buffer creation string_view", "[buffer]")
{
    std::wstring dstr(10, L'a');
    std::wstring_view d = dstr;
    zmq::const_buffer b = zmq::buffer(d);
    CHECK(b.size() == d.size() * sizeof(wchar_t));
    CHECK(b.data() == d.data());
    zmq::const_buffer b2 = zmq::buffer(d, 4);
    CHECK(b2.size() == 4);
    CHECK(b2.data() == d.data());
}
#endif

TEST_CASE("const_buffer creation with str_buffer", "[buffer]")
{
    const wchar_t wd[10] = {};
    zmq::const_buffer b = zmq::str_buffer(wd);
    CHECK(b.size() == 9 * sizeof(wchar_t));
    CHECK(b.data() == static_cast<const wchar_t*>(wd));

    zmq::const_buffer b2_null = zmq::buffer("hello");
    constexpr zmq::const_buffer b2 = zmq::str_buffer("hello");
    CHECK(b2_null.size() == 6);
    CHECK(b2.size() == 5);
    CHECK(std::string(static_cast<const char*>(b2.data()), b2.size()) == "hello");
}

TEST_CASE("const_buffer creation with zbuf string literal char", "[buffer]")
{
    using namespace zmq::literals;
    constexpr zmq::const_buffer b = "hello"_zbuf;
    CHECK(b.size() == 5);
    CHECK(std::memcmp(b.data(), "hello", b.size()) == 0);
}

TEST_CASE("const_buffer creation with zbuf string literal wchar_t", "[buffer]")
{
    using namespace zmq::literals;
    constexpr zmq::const_buffer b = L"hello"_zbuf;
    CHECK(b.size() == 5 * sizeof(wchar_t));
    CHECK(std::memcmp(b.data(), L"hello", b.size()) == 0);
}

TEST_CASE("const_buffer creation with zbuf string literal char16_t", "[buffer]")
{
    using namespace zmq::literals;
    constexpr zmq::const_buffer b = u"hello"_zbuf;
    CHECK(b.size() == 5 * sizeof(char16_t));
    CHECK(std::memcmp(b.data(), u"hello", b.size()) == 0);
}

TEST_CASE("const_buffer creation with zbuf string literal char32_t", "[buffer]")
{
    using namespace zmq::literals;
    constexpr zmq::const_buffer b = U"hello"_zbuf;
    CHECK(b.size() == 5 * sizeof(char32_t));
    CHECK(std::memcmp(b.data(), U"hello", b.size()) == 0);
}

TEST_CASE("buffer of structs", "[buffer]")
{
    struct some_pod
    {
        int64_t val;
        char arr[8];
    };
    struct some_non_pod
    {
        int64_t val;
        char arr[8];
        std::vector<int> s; // not trivially copyable
    };
    static_assert(zmq::detail::is_pod_like<some_pod>::value, "");
    static_assert(!zmq::detail::is_pod_like<some_non_pod>::value, "");
    std::array<some_pod, 1> d;
    zmq::mutable_buffer b = zmq::buffer(d);
    CHECK(b.size() == d.size() * sizeof(some_pod));
    CHECK(b.data() == d.data());
}

#endif
