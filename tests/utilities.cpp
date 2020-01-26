#include <catch.hpp>
#include <zmq.hpp>

#if defined(ZMQ_CPP11) && !defined(ZMQ_CPP11_PARTIAL)

namespace test_ns
{
struct T_nr
{
};

struct T_mr
{
    void *begin() const noexcept { return nullptr; }
    void *end() const noexcept { return nullptr; }
};

struct T_fr
{
};

inline void *begin(const T_fr &) noexcept
{
    return nullptr;
}

inline void *end(const T_fr &) noexcept
{
    return nullptr;
}

struct T_mfr
{
    void *begin() const noexcept { return nullptr; }
    void *end() const noexcept { return nullptr; }
};

inline void *begin(const T_mfr &) noexcept
{
    return nullptr;
}

inline void *end(const T_mfr &) noexcept
{
    return nullptr;
}

// types with associated namespace std
struct T_assoc_ns_nr : std::exception
{
};

struct T_assoc_ns_mr : std::exception
{
    void *begin() const noexcept { return nullptr; }
    void *end() const noexcept { return nullptr; }
};

struct T_assoc_ns_fr : std::exception
{
};

inline void *begin(const T_assoc_ns_fr &) noexcept
{
    return nullptr;
}

inline void *end(const T_assoc_ns_fr &) noexcept
{
    return nullptr;
}

struct T_assoc_ns_mfr : std::exception
{
    void *begin() const noexcept { return nullptr; }
    void *end() const noexcept { return nullptr; }
};

inline void *begin(const T_assoc_ns_mfr &) noexcept
{
    return nullptr;
}

inline void *end(const T_assoc_ns_mfr &) noexcept
{
    return nullptr;
}
} // namespace test_ns

TEST_CASE("range SFINAE", "[utilities]")
{
    CHECK(!zmq::detail::is_range<int>::value);
    CHECK(zmq::detail::is_range<std::string>::value);
    CHECK(zmq::detail::is_range<std::string &>::value);
    CHECK(zmq::detail::is_range<const std::string &>::value);
    CHECK(zmq::detail::is_range<decltype("hello")>::value);
    CHECK(zmq::detail::is_range<std::initializer_list<int>>::value);

    CHECK(!zmq::detail::is_range<test_ns::T_nr>::value);
    CHECK(zmq::detail::is_range<test_ns::T_mr>::value);
    CHECK(zmq::detail::is_range<test_ns::T_fr>::value);
    CHECK(zmq::detail::is_range<test_ns::T_mfr>::value);

    CHECK(!zmq::detail::is_range<test_ns::T_assoc_ns_nr>::value);
    CHECK(zmq::detail::is_range<test_ns::T_assoc_ns_mr>::value);
    CHECK(zmq::detail::is_range<test_ns::T_assoc_ns_fr>::value);
    CHECK(zmq::detail::is_range<test_ns::T_assoc_ns_mfr>::value);
}

#endif
