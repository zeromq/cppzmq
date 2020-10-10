#include <catch.hpp>
#include <zmq.hpp>
#ifdef ZMQ_CPP11
#include <future>
#endif

#if (__cplusplus >= 201703L)
static_assert(std::is_nothrow_swappable<zmq::socket_t>::value,
              "socket_t should be nothrow swappable");
#endif

TEST_CASE("socket default ctor", "[socket]")
{
    zmq::socket_t socket;
}

TEST_CASE("socket create destroy", "[socket]")
{
    zmq::context_t context;
    zmq::socket_t socket(context, ZMQ_ROUTER);
}

#ifdef ZMQ_CPP11
TEST_CASE("socket create assign", "[socket]")
{
    zmq::context_t context;
    zmq::socket_t socket(context, ZMQ_ROUTER);
    CHECK(static_cast<bool>(socket));
    CHECK(socket.handle() != nullptr);
    socket = {};
    CHECK(!static_cast<bool>(socket));
    CHECK(socket.handle() == nullptr);
}

TEST_CASE("socket create by enum and destroy", "[socket]")
{
    zmq::context_t context;
    zmq::socket_t socket(context, zmq::socket_type::router);
}

TEST_CASE("socket swap", "[socket]")
{
    zmq::context_t context;
    zmq::socket_t socket1(context, zmq::socket_type::router);
    zmq::socket_t socket2(context, zmq::socket_type::dealer);
    using std::swap;
    swap(socket1, socket2);
}

#ifdef ZMQ_CPP11
TEST_CASE("socket options", "[socket]")
{
    zmq::context_t context;
    zmq::socket_t socket(context, zmq::socket_type::router);

#ifdef ZMQ_IMMEDIATE
    socket.set(zmq::sockopt::immediate, 0);
    socket.set(zmq::sockopt::immediate, false);
    CHECK(socket.get(zmq::sockopt::immediate) == false);
    // unit out of range
    CHECK_THROWS_AS(socket.set(zmq::sockopt::immediate, 80), const zmq::error_t &);
#endif
#ifdef ZMQ_LINGER
    socket.set(zmq::sockopt::linger, 55);
    CHECK(socket.get(zmq::sockopt::linger) == 55);
#endif
#ifdef ZMQ_ROUTING_ID
    const std::string id = "foobar";
    socket.set(zmq::sockopt::routing_id, "foobar");
    socket.set(zmq::sockopt::routing_id, zmq::buffer(id));
    socket.set(zmq::sockopt::routing_id, id);
#if CPPZMQ_HAS_STRING_VIEW
    socket.set(zmq::sockopt::routing_id, std::string_view{id});
#endif

    std::string id_ret(10, ' ');
    auto size = socket.get(zmq::sockopt::routing_id, zmq::buffer(id_ret));
    id_ret.resize(size);
    CHECK(id == id_ret);
    auto stropt = socket.get(zmq::sockopt::routing_id);
    CHECK(id == stropt);

    std::string id_ret_small(3, ' ');
    // truncated
    CHECK_THROWS_AS(socket.get(zmq::sockopt::routing_id, zmq::buffer(id_ret_small)),
                    const zmq::error_t &);
#endif
}

template<class T>
void check_array_opt(T opt,
                     zmq::socket_t &sock,
                     std::string info,
                     bool set_only = false)
{
    const std::string val = "foobar";
    INFO("setting " + info);
    sock.set(opt, val);
    if (set_only)
        return;

    INFO("getting " + info);
    auto s = sock.get(opt);
    CHECK(s == val);
}

template<class T>
void check_array_opt_get(T opt, zmq::socket_t &sock, std::string info)
{
    INFO("getting " + info);
    (void) sock.get(opt);
}

#if ZMQ_VERSION >= ZMQ_MAKE_VERSION(4, 0, 0)
template<class T> void check_bin_z85(T opt, zmq::socket_t &sock, std::string str_val)
{
    std::vector<uint8_t> bin_val(32);
    const auto dret = zmq_z85_decode(bin_val.data(), str_val.c_str());
    CHECK(dret != nullptr);

    sock.set(opt, str_val);
    sock.set(opt, zmq::buffer(bin_val));
    auto sv = sock.get(opt);
    CHECK(sv == str_val);

    auto bv = sock.get(opt, 32);
    REQUIRE(bv.size() == bin_val.size());
    CHECK(std::memcmp(bv.data(), bin_val.data(), bin_val.size()) == 0);
}
#endif

TEST_CASE("socket check array options", "[socket]")
{
    zmq::context_t context;
    zmq::socket_t router(context, zmq::socket_type::router);
    zmq::socket_t xpub(context, zmq::socket_type::xpub);
    zmq::socket_t sub(context, zmq::socket_type::sub);

#ifdef ZMQ_BINDTODEVICE
// requires setting CAP_NET_RAW
//check_array_opt(zmq::sockopt::bindtodevice, router, "bindtodevice");
#endif
#ifdef ZMQ_CONNECT_ROUTING_ID
    check_array_opt(zmq::sockopt::connect_routing_id, router, "connect_routing_id",
                    true);
#endif
#ifdef ZMQ_LAST_ENDPOINT
    check_array_opt_get(zmq::sockopt::last_endpoint, router, "last_endpoint");
#endif
#ifdef ZMQ_METADATA
    router.set(zmq::sockopt::metadata, zmq::str_buffer("X-foo:bar"));
#endif
#ifdef ZMQ_PLAIN_PASSWORD
    check_array_opt(zmq::sockopt::plain_password, router, "plain_password");
#endif
#ifdef ZMQ_PLAIN_USERNAME
    check_array_opt(zmq::sockopt::plain_username, router, "plain_username");
#endif
#ifdef ZMQ_ROUTING_ID
    check_array_opt(zmq::sockopt::routing_id, router, "routing_id");
#endif
#ifdef ZMQ_SOCKS_PROXY
    check_array_opt(zmq::sockopt::socks_proxy, router, "socks_proxy");
#endif
#ifdef ZMQ_SUBSCRIBE
    check_array_opt(zmq::sockopt::subscribe, sub, "subscribe", true);
#endif
#ifdef ZMQ_UNSUBSCRIBE
    check_array_opt(zmq::sockopt::unsubscribe, sub, "unsubscribe", true);
#endif
#ifdef ZMQ_XPUB_WELCOME_MSG
    check_array_opt(zmq::sockopt::xpub_welcome_msg, xpub, "xpub_welcome_msg", true);
#endif
#ifdef ZMQ_ZAP_DOMAIN
    check_array_opt(zmq::sockopt::zap_domain, router, "zap_domain");
#endif

// curve
#if ZMQ_VERSION >= ZMQ_MAKE_VERSION(4, 0, 0) && defined(ZMQ_HAS_CAPABILITIES)
    if (zmq_has("curve") == 1) {
        const std::string spk = "rq:rM>}U?@Lns47E1%kR.o@n%FcmmsL/@{H8]yf7";
        const std::string ssk = "JTKVSB%%)wK0E.X)V>+}o?pNmC{O&4W4b!Ni{Lh6";
        const std::string cpk = "Yne@$w-vo<fVvi]a<NY6T1ed:M$fCG*[IaLV{hID";
        const std::string csk = "D:)Q[IlAW!ahhC2ac:9*A}h:p?([4%wOTJ%JR%cs";

        zmq::socket_t curve_server(context, zmq::socket_type::router);
        curve_server.set(zmq::sockopt::curve_server, true);
        CHECK(curve_server.get(zmq::sockopt::curve_server));
        check_bin_z85(zmq::sockopt::curve_secretkey, curve_server, ssk);

        zmq::socket_t curve_client(context, zmq::socket_type::router);
        curve_client.set(zmq::sockopt::curve_server, false);
        CHECK_FALSE(curve_client.get(zmq::sockopt::curve_server));
        check_bin_z85(zmq::sockopt::curve_serverkey, curve_client, spk);
        check_bin_z85(zmq::sockopt::curve_publickey, curve_client, cpk);
        check_bin_z85(zmq::sockopt::curve_secretkey, curve_client, csk);
    }
#endif

// gssapi
#if ZMQ_VERSION >= ZMQ_MAKE_VERSION(4, 1, 0) && defined(ZMQ_HAS_CAPABILITIES)
    if (zmq_has("gssapi") == 1 && false) // TODO enable
    {
        zmq::socket_t gss_server(context, zmq::socket_type::router);
        gss_server.set(zmq::sockopt::gssapi_server, true);
        CHECK(gss_server.get(zmq::sockopt::gssapi_server) == 1);
        gss_server.set(zmq::sockopt::gssapi_plaintext, false);
        CHECK(gss_server.get(zmq::sockopt::gssapi_plaintext) == 0);
        check_array_opt(zmq::sockopt::gssapi_principal, gss_server,
                        "gssapi_principal");

        zmq::socket_t gss_client(context, zmq::socket_type::router);
        CHECK(gss_client.get(zmq::sockopt::gssapi_server) == 0);
        check_array_opt(zmq::sockopt::gssapi_principal, gss_client,
                        "gssapi_principal");
        check_array_opt(zmq::sockopt::gssapi_service_principal, gss_client,
                        "gssapi_service_principal");
    }
#endif
}

template<class T, class Opt>
void check_integral_opt(Opt opt,
                        zmq::socket_t &sock,
                        std::string info,
                        bool set_only = false)
{
    const T val = 1;
    INFO("setting " + info);
    sock.set(opt, val);
    if (set_only)
        return;

    INFO("getting " + info);
    auto s = sock.get(opt);
    CHECK(s == val);
}

template<class T, class Opt>
void check_integral_opt_get(Opt opt, zmq::socket_t &sock, std::string info)
{
    INFO("getting " + info);
    (void) sock.get(opt);
}

TEST_CASE("socket check integral options", "[socket]")
{
    zmq::context_t context;
    zmq::socket_t router(context, zmq::socket_type::router);
    zmq::socket_t xpub(context, zmq::socket_type::xpub);
    zmq::socket_t req(context, zmq::socket_type::req);
#ifdef ZMQ_STREAM_NOTIFY
    zmq::socket_t stream(context, zmq::socket_type::stream);
#endif

#ifdef ZMQ_AFFINITY
    check_integral_opt<uint64_t>(zmq::sockopt::affinity, router, "affinity");
#endif
#ifdef ZMQ_BACKLOG
    check_integral_opt<int>(zmq::sockopt::backlog, router, "backlog");
#endif
#ifdef ZMQ_CONFLATE
    check_integral_opt<int>(zmq::sockopt::conflate, router, "conflate");
#endif
#ifdef ZMQ_CONNECT_TIMEOUT
    check_integral_opt<int>(zmq::sockopt::connect_timeout, router,
                            "connect_timeout");
#endif
#ifdef ZMQ_EVENTS
    check_integral_opt_get<int>(zmq::sockopt::events, router, "events");
#endif
#ifdef ZMQ_FD
    check_integral_opt_get<zmq::fd_t>(zmq::sockopt::fd, router, "fd");
#endif
#ifdef ZMQ_HANDSHAKE_IVL
    check_integral_opt<int>(zmq::sockopt::handshake_ivl, router, "handshake_ivl");
#endif
#ifdef ZMQ_HEARTBEAT_IVL
    check_integral_opt<int>(zmq::sockopt::heartbeat_ivl, router, "heartbeat_ivl");
#endif
#ifdef ZMQ_HEARTBEAT_TIMEOUT
    check_integral_opt<int>(zmq::sockopt::heartbeat_timeout, router,
                            "heartbeat_timeout");
#endif
#ifdef ZMQ_HEARTBEAT_TTL
    router.set(zmq::sockopt::heartbeat_ttl, 100);
    CHECK(router.get(zmq::sockopt::heartbeat_ttl) == 100);
#endif
#ifdef ZMQ_IMMEDIATE
    check_integral_opt<int>(zmq::sockopt::immediate, router, "immediate");
#endif
#ifdef ZMQ_INVERT_MATCHING
    check_integral_opt<int>(zmq::sockopt::invert_matching, router,
                            "invert_matching");
#endif
#ifdef ZMQ_IPV6
    check_integral_opt<int>(zmq::sockopt::ipv6, router, "ipv6");
#endif
#ifdef ZMQ_LINGER
    check_integral_opt<int>(zmq::sockopt::linger, router, "linger");
#endif
#ifdef ZMQ_MAXMSGSIZE
    check_integral_opt<int64_t>(zmq::sockopt::maxmsgsize, router, "maxmsgsize");
#endif
#ifdef ZMQ_MECHANISM
    check_integral_opt_get<int>(zmq::sockopt::mechanism, router, "mechanism");
#endif
#ifdef ZMQ_MULTICAST_HOPS
    check_integral_opt<int>(zmq::sockopt::multicast_hops, router, "multicast_hops");
#endif
#ifdef ZMQ_MULTICAST_LOOP
    check_integral_opt<int>(zmq::sockopt::multicast_loop, router, "multicast_loop");
#endif
#ifdef ZMQ_MULTICAST_MAXTPDU
    check_integral_opt<int>(zmq::sockopt::multicast_maxtpdu, router,
                            "multicast_maxtpdu");
#endif
#ifdef ZMQ_PLAIN_SERVER
    check_integral_opt<int>(zmq::sockopt::plain_server, router, "plain_server");
#endif
#ifdef ZMQ_USE_FD
    check_integral_opt<int>(zmq::sockopt::use_fd, router, "use_fd");
#endif
#ifdef ZMQ_PROBE_ROUTER
    check_integral_opt<int>(zmq::sockopt::probe_router, router, "probe_router",
                            true);
#endif
#ifdef ZMQ_RATE
    check_integral_opt<int>(zmq::sockopt::rate, router, "rate");
#endif
#ifdef ZMQ_RCVBUF
    check_integral_opt<int>(zmq::sockopt::rcvbuf, router, "rcvbuf");
#endif
#ifdef ZMQ_RCVHWM
    check_integral_opt<int>(zmq::sockopt::rcvhwm, router, "rcvhwm");
#endif
#ifdef ZMQ_RCVMORE
    check_integral_opt_get<int>(zmq::sockopt::rcvmore, router, "rcvmore");
#endif
#ifdef ZMQ_RCVTIMEO
    check_integral_opt<int>(zmq::sockopt::rcvtimeo, router, "rcvtimeo");
#endif
#ifdef ZMQ_RECONNECT_IVL
    check_integral_opt<int>(zmq::sockopt::reconnect_ivl, router, "reconnect_ivl");
#endif
#ifdef ZMQ_RECONNECT_IVL_MAX
    check_integral_opt<int>(zmq::sockopt::reconnect_ivl_max, router,
                            "reconnect_ivl_max");
#endif
#ifdef ZMQ_RECOVERY_IVL
    check_integral_opt<int>(zmq::sockopt::recovery_ivl, router, "recovery_ivl");
#endif
#ifdef ZMQ_REQ_CORRELATE
    check_integral_opt<int>(zmq::sockopt::req_correlate, req, "req_correlate", true);
#endif
#ifdef ZMQ_REQ_RELAXED
    check_integral_opt<int>(zmq::sockopt::req_relaxed, req, "req_relaxed", true);
#endif
#ifdef ZMQ_ROUTER_HANDOVER
    check_integral_opt<int>(zmq::sockopt::router_handover, router, "router_handover",
                            true);
#endif
#ifdef ZMQ_ROUTER_MANDATORY
    check_integral_opt<int>(zmq::sockopt::router_mandatory, router,
                            "router_mandatory", true);
#endif
#ifdef ZMQ_ROUTER_NOTIFY
    check_integral_opt<int>(zmq::sockopt::router_notify, router, "router_notify");
#endif
#ifdef ZMQ_SNDBUF
    check_integral_opt<int>(zmq::sockopt::sndbuf, router, "sndbuf");
#endif
#ifdef ZMQ_SNDHWM
    check_integral_opt<int>(zmq::sockopt::sndhwm, router, "sndhwm");
#endif
#ifdef ZMQ_SNDTIMEO
    check_integral_opt<int>(zmq::sockopt::sndtimeo, router, "sndtimeo");
#endif
#ifdef ZMQ_STREAM_NOTIFY
    check_integral_opt<int>(zmq::sockopt::stream_notify, stream, "stream_notify",
                            true);
#endif
#ifdef ZMQ_TCP_KEEPALIVE
    check_integral_opt<int>(zmq::sockopt::tcp_keepalive, router, "tcp_keepalive");
#endif
#ifdef ZMQ_TCP_KEEPALIVE_CNT
    check_integral_opt<int>(zmq::sockopt::tcp_keepalive_cnt, router,
                            "tcp_keepalive_cnt");
#endif
#ifdef ZMQ_TCP_KEEPALIVE_IDLE
    check_integral_opt<int>(zmq::sockopt::tcp_keepalive_idle, router,
                            "tcp_keepalive_idle");
#endif
#ifdef ZMQ_TCP_KEEPALIVE_INTVL
    check_integral_opt<int>(zmq::sockopt::tcp_keepalive_intvl, router,
                            "tcp_keepalive_intvl");
#endif
#ifdef ZMQ_TCP_MAXRT
    check_integral_opt<int>(zmq::sockopt::tcp_maxrt, router, "tcp_maxrt");
#endif
#ifdef ZMQ_THREAD_SAFE
    check_integral_opt_get<bool>(zmq::sockopt::thread_safe, router, "thread_safe");
#endif
#ifdef ZMQ_TOS
    check_integral_opt<int>(zmq::sockopt::tos, router, "tos");
#endif
#ifdef ZMQ_TYPE
    check_integral_opt_get<int>(zmq::sockopt::type, router, "type");
#endif

#ifdef ZMQ_HAVE_VMCI
#ifdef ZMQ_VMCI_BUFFER_SIZE
    check_integral_opt<uint64_t>(zmq::sockopt::vmci_buffer_size, router,
                                 "vmci_buffer_size");
#endif
#ifdef ZMQ_VMCI_BUFFER_MIN_SIZE
    check_integral_opt<uint64_t>(zmq::sockopt::vmci_buffer_min_size, router,
                                 "vmci_buffer_min_size");
#endif
#ifdef ZMQ_VMCI_BUFFER_MAX_SIZE
    check_integral_opt<uint64_t>(zmq::sockopt::vmci_buffer_max_size, router,
                                 "vmci_buffer_max_size");
#endif
#ifdef ZMQ_VMCI_CONNECT_TIMEOUT
    check_integral_opt<int>(zmq::sockopt::vmci_connect_timeout, router,
                            "vmci_connect_timeout");
#endif
#endif

#ifdef ZMQ_XPUB_VERBOSE
    check_integral_opt<int>(zmq::sockopt::xpub_verbose, xpub, "xpub_verbose", true);
#endif
#ifdef ZMQ_XPUB_VERBOSER
    check_integral_opt<int>(zmq::sockopt::xpub_verboser, xpub, "xpub_verboser",
                            true);
#endif
#ifdef ZMQ_XPUB_MANUAL
    check_integral_opt<int>(zmq::sockopt::xpub_manual, xpub, "xpub_manual", true);
#endif
#ifdef ZMQ_XPUB_NODROP
    check_integral_opt<int>(zmq::sockopt::xpub_nodrop, xpub, "xpub_nodrop", true);
#endif
#ifdef ZMQ_ZAP_ENFORCE_DOMAIN
    check_integral_opt<int>(zmq::sockopt::zap_enforce_domain, router,
                            "zap_enforce_domain");
#endif
}

#endif

TEST_CASE("socket flags", "[socket]")
{
    CHECK((zmq::recv_flags::dontwait | zmq::recv_flags::none)
          == static_cast<zmq::recv_flags>(ZMQ_DONTWAIT | 0));
    CHECK((zmq::recv_flags::dontwait & zmq::recv_flags::none)
          == static_cast<zmq::recv_flags>(ZMQ_DONTWAIT & 0));
    CHECK((zmq::recv_flags::dontwait ^ zmq::recv_flags::none)
          == static_cast<zmq::recv_flags>(ZMQ_DONTWAIT ^ 0));
    CHECK(~zmq::recv_flags::dontwait == static_cast<zmq::recv_flags>(~ZMQ_DONTWAIT));

    CHECK((zmq::send_flags::dontwait | zmq::send_flags::sndmore)
          == static_cast<zmq::send_flags>(ZMQ_DONTWAIT | ZMQ_SNDMORE));
    CHECK((zmq::send_flags::dontwait & zmq::send_flags::sndmore)
          == static_cast<zmq::send_flags>(ZMQ_DONTWAIT & ZMQ_SNDMORE));
    CHECK((zmq::send_flags::dontwait ^ zmq::send_flags::sndmore)
          == static_cast<zmq::send_flags>(ZMQ_DONTWAIT ^ ZMQ_SNDMORE));
    CHECK(~zmq::send_flags::dontwait == static_cast<zmq::send_flags>(~ZMQ_DONTWAIT));
}

TEST_CASE("socket readme example", "[socket]")
{
    zmq::context_t ctx;
    zmq::socket_t sock(ctx, zmq::socket_type::push);
    sock.bind("inproc://test");
    sock.send(zmq::str_buffer("Hello, world"), zmq::send_flags::dontwait);
}
#endif

TEST_CASE("socket sends and receives const buffer", "[socket]")
{
    zmq::context_t context;
    zmq::socket_t sender(context, ZMQ_PAIR);
    zmq::socket_t receiver(context, ZMQ_PAIR);
    receiver.bind("inproc://test");
    sender.connect("inproc://test");
    const char *str = "Hi";

#ifdef ZMQ_CPP11
    CHECK(2 == *sender.send(zmq::buffer(str, 2)));
    char buf[2];
    const auto res = receiver.recv(zmq::buffer(buf));
    CHECK(res);
    CHECK(!res->truncated());
    CHECK(2 == res->size);
#else
    CHECK(2 == sender.send(str, 2));
    char buf[2];
    CHECK(2 == receiver.recv(buf, 2));
#endif
    CHECK(0 == memcmp(buf, str, 2));
}

#ifdef ZMQ_CPP11

TEST_CASE("socket send none sndmore", "[socket]")
{
    zmq::context_t context;
    zmq::socket_t s(context, zmq::socket_type::router);
    s.bind("inproc://test");

    std::vector<char> buf(4);
    auto res = s.send(zmq::buffer(buf), zmq::send_flags::sndmore);
    CHECK(res);
    CHECK(*res == buf.size());
    res = s.send(zmq::buffer(buf));
    CHECK(res);
    CHECK(*res == buf.size());
}

TEST_CASE("socket send dontwait", "[socket]")
{
    zmq::context_t context;
    zmq::socket_t s(context, zmq::socket_type::push);
    s.bind("inproc://test");

    std::vector<char> buf(4);
    auto res = s.send(zmq::buffer(buf), zmq::send_flags::dontwait);
    CHECK(!res);
    res =
      s.send(zmq::buffer(buf), zmq::send_flags::dontwait | zmq::send_flags::sndmore);
    CHECK(!res);

    zmq::message_t msg;
    auto resm = s.send(msg, zmq::send_flags::dontwait);
    CHECK(!resm);
    CHECK(msg.size() == 0);
}

TEST_CASE("socket send exception", "[socket]")
{
    zmq::context_t context;
    zmq::socket_t s(context, zmq::socket_type::pull);
    s.bind("inproc://test");

    std::vector<char> buf(4);
    CHECK_THROWS_AS(s.send(zmq::buffer(buf)), const zmq::error_t &);
}

TEST_CASE("socket recv none", "[socket]")
{
    zmq::context_t context;
    zmq::socket_t s(context, zmq::socket_type::pair);
    zmq::socket_t s2(context, zmq::socket_type::pair);
    s2.bind("inproc://test");
    s.connect("inproc://test");

    std::vector<char> sbuf(4);
    const auto res_send = s2.send(zmq::buffer(sbuf));
    CHECK(res_send);
    CHECK(res_send.has_value());

    std::vector<char> buf(2);
    const auto res = s.recv(zmq::buffer(buf));
    CHECK(res.has_value());
    CHECK(res->truncated());
    CHECK(res->untruncated_size == sbuf.size());
    CHECK(res->size == buf.size());

    const auto res_send2 = s2.send(zmq::buffer(sbuf));
    CHECK(res_send2.has_value());
    std::vector<char> buf2(10);
    const auto res2 = s.recv(zmq::buffer(buf2));
    CHECK(res2.has_value());
    CHECK(!res2->truncated());
    CHECK(res2->untruncated_size == sbuf.size());
    CHECK(res2->size == sbuf.size());
}

TEST_CASE("socket send recv message_t", "[socket]")
{
    zmq::context_t context;
    zmq::socket_t s(context, zmq::socket_type::pair);
    zmq::socket_t s2(context, zmq::socket_type::pair);
    s2.bind("inproc://test");
    s.connect("inproc://test");

    zmq::message_t smsg(10);
    const auto res_send = s2.send(smsg, zmq::send_flags::none);
    CHECK(res_send);
    CHECK(*res_send == 10);
    CHECK(smsg.size() == 0);

    zmq::message_t rmsg;
    const auto res = s.recv(rmsg);
    CHECK(res);
    CHECK(*res == 10);
    CHECK(res.value() == 10);
    CHECK(rmsg.size() == *res);
}

TEST_CASE("socket send recv message_t by pointer", "[socket]")
{
    zmq::context_t context;
    zmq::socket_t s(context, zmq::socket_type::pair);
    zmq::socket_t s2(context, zmq::socket_type::pair);
    s2.bind("inproc://test");
    s.connect("inproc://test");

    zmq::message_t smsg(size_t{10});
    const auto res_send = s2.send(smsg, zmq::send_flags::none);
    CHECK(res_send);
    CHECK(*res_send == 10);
    CHECK(smsg.size() == 0);

    zmq::message_t rmsg;
    const bool res = s.recv(&rmsg);
    CHECK(res);
}

TEST_CASE("socket recv dontwait", "[socket]")
{
    zmq::context_t context;
    zmq::socket_t s(context, zmq::socket_type::pull);
    s.bind("inproc://test");

    std::vector<char> buf(4);
    constexpr auto flags = zmq::recv_flags::none | zmq::recv_flags::dontwait;
    auto res = s.recv(zmq::buffer(buf), flags);
    CHECK(!res);

    zmq::message_t msg;
    auto resm = s.recv(msg, flags);
    CHECK(!resm);
    CHECK_THROWS_AS(resm.value(), const std::exception &);
    CHECK(msg.size() == 0);
}

TEST_CASE("socket recv exception", "[socket]")
{
    zmq::context_t context;
    zmq::socket_t s(context, zmq::socket_type::push);
    s.bind("inproc://test");

    std::vector<char> buf(4);
    CHECK_THROWS_AS(s.recv(zmq::buffer(buf)), const zmq::error_t &);
}

TEST_CASE("socket proxy", "[socket]")
{
    zmq::context_t context;
    zmq::socket_t front(context, ZMQ_ROUTER);
    zmq::socket_t back(context, ZMQ_ROUTER);
    zmq::socket_t capture(context, ZMQ_DEALER);
    front.bind("inproc://test1");
    back.bind("inproc://test2");
    capture.bind("inproc://test3");
    auto f = std::async(std::launch::async, [&]() {
        auto s1 = std::move(front);
        auto s2 = std::move(back);
        auto s3 = std::move(capture);
        try {
            zmq::proxy(s1, s2, zmq::socket_ref(s3));
        }
        catch (const zmq::error_t &e) {
            return e.num() == ETERM;
        }
        return false;
    });
    context.close();
    CHECK(f.get());
}

TEST_CASE("socket proxy steerable", "[socket]")
{
    zmq::context_t context;
    zmq::socket_t front(context, ZMQ_ROUTER);
    zmq::socket_t back(context, ZMQ_ROUTER);
    zmq::socket_t control(context, ZMQ_SUB);
    front.bind("inproc://test1");
    back.bind("inproc://test2");
    control.connect("inproc://test3");
    auto f = std::async(std::launch::async, [&]() {
        auto s1 = std::move(front);
        auto s2 = std::move(back);
        auto s3 = std::move(control);
        try {
            zmq::proxy_steerable(s1, s2, zmq::socket_ref(), s3);
        }
        catch (const zmq::error_t &e) {
            return e.num() == ETERM;
        }
        return false;
    });
    context.close();
    CHECK(f.get());
}
#endif
