#include <jb/itch5/make_socket_udp_send.hpp>

#include <jb/itch5/testing/mock_udp_socket.hpp>
#include <boost/test/unit_test.hpp>

/// @test Verify that jb::itch5::make_socket_udp_send compiles.
BOOST_AUTO_TEST_CASE(itch5_make_socket_udp_send_compile) {
  boost::asio::io_service io;
  jb::itch5::udp_sender_config cfg;
  auto socket =
      jb::itch5::make_socket_udp_send(io, cfg.address("127.0.0.1").port(40000));
  BOOST_CHECK(socket.is_open());
}

using jb::itch5::testing::mock_udp_socket;

/// @test Create a simple unicast socket on the default interface
BOOST_AUTO_TEST_CASE(itch5_make_socket_udp_send_basic) {
  using namespace ::testing;
  NiceMock<mock_udp_socket> socket;
  EXPECT_CALL(socket, open(_)).Times(1);
  EXPECT_CALL(socket, bind(_)).Times(1);
  EXPECT_CALL(
      socket, set_option(An<boost::asio::ip::multicast::join_group const&>()))
      .Times(0);
  EXPECT_CALL(
      socket,
      set_option(An<boost::asio::ip::multicast::enable_loopback const&>()))
      .Times(0);
  jb::itch5::detail::setup_socket_udp_send(
      socket, jb::itch5::udp_sender_config().address("::1").port(50000));
}

/// @test Create a IPv4 multicast socket on the default interface.
BOOST_AUTO_TEST_CASE(itch5_make_socket_udp_send_multicast_ipv4) {
  using namespace ::testing;
  NiceMock<mock_udp_socket> socket;
  EXPECT_CALL(socket, open(_)).Times(1);
  EXPECT_CALL(
      socket,
      bind(boost::asio::ip::udp::endpoint(boost::asio::ip::address_v4(), 0)))
      .Times(1);
  EXPECT_CALL(
      socket,
      set_option(An<boost::asio::ip::multicast::enable_loopback const&>()))
      .Times(1);

  jb::itch5::detail::setup_socket_udp_send(
      socket,
      jb::itch5::udp_sender_config().address("239.128.1.1").port(50000));
}

/// @test Create a IPv6 multicast socket on the default interface.
BOOST_AUTO_TEST_CASE(itch5_make_socket_udp_send_multicast_ipv6) {
  using namespace ::testing;
  NiceMock<mock_udp_socket> socket;
  EXPECT_CALL(socket, open(_)).Times(1);
  EXPECT_CALL(
      socket,
      bind(boost::asio::ip::udp::endpoint(boost::asio::ip::address_v6(), 0)))
      .Times(1);
  EXPECT_CALL(
      socket,
      set_option(An<boost::asio::ip::multicast::enable_loopback const&>()))
      .Times(1);

  jb::itch5::detail::setup_socket_udp_send(
      socket, jb::itch5::udp_sender_config().address("ff05::").port(50000));
}

/// @test Create a multicast socket on an specific interface ...
BOOST_AUTO_TEST_CASE(itch5_make_socket_udp_send_multicast_options) {
  using namespace ::testing;
  NiceMock<mock_udp_socket> v6s;
  EXPECT_CALL(v6s, open(_)).Times(1);
  EXPECT_CALL(
      v6s,
      bind(boost::asio::ip::udp::endpoint(boost::asio::ip::address_v6(), 0)))
      .Times(1);
  EXPECT_CALL(
      v6s, set_option(An<boost::asio::ip::multicast::enable_loopback const&>()))
      .Times(1);
  EXPECT_CALL(
      v6s, set_option(Matcher<boost::asio::ip::multicast::hops const&>(
               Truly([](boost::asio::ip::multicast::hops const& h) {
                 return h.value() == 10;
               }))))
      .Times(1);

  jb::itch5::detail::setup_socket_udp_send(
      v6s, jb::itch5::udp_sender_config()
               .address("ff05::")
               .port(50000)
               .enable_loopback(true)
               .hops(10)
               .outbound_interface("2"));

  NiceMock<mock_udp_socket> v4s;
  EXPECT_CALL(v4s, open(_)).Times(1);
  EXPECT_CALL(
      v4s,
      bind(boost::asio::ip::udp::endpoint(boost::asio::ip::address_v4(), 0)))
      .Times(1);
  EXPECT_CALL(
      v4s, set_option(boost::asio::ip::multicast::enable_loopback(true)))
      .Times(1);
  EXPECT_CALL(v4s, set_option(An<boost::asio::ip::multicast::hops const&>()))
      .Times(0);
  jb::itch5::detail::setup_socket_udp_send(
      v4s, jb::itch5::udp_sender_config()
               .address("239.128.1.1")
               .port(50000)
               .enable_loopback(true)
               .hops(-1)
               .outbound_interface("127.0.0.1"));
}

/// @test Create a multicast socket on an specific interface.
BOOST_AUTO_TEST_CASE(itch5_make_socket_udp_send_multicast_options_errors) {
  boost::asio::io_service io;
  BOOST_CHECK_THROW(
      jb::itch5::make_socket_udp_send(
          io, jb::itch5::udp_sender_config()
                  .address("ff05::")
                  .port(50000)
                  .enable_loopback(true)
                  .hops(10)
                  .outbound_interface("abcd")),
      std::runtime_error);
}

/// @test Create a multicast socket on an specific interface.
BOOST_AUTO_TEST_CASE(itch5_make_socket_udp_send_unicast_options) {
  using namespace ::testing;
  NiceMock<mock_udp_socket> socket;
  EXPECT_CALL(socket, open(_)).Times(1);
  EXPECT_CALL(
      socket,
      bind(boost::asio::ip::udp::endpoint(boost::asio::ip::address_v4(), 0)))
      .Times(1);
  EXPECT_CALL(
      socket, set_option(Matcher<boost::asio::ip::unicast::hops const&>(
                  Truly([](boost::asio::ip::unicast::hops const& h) {
                    return h.value() == 10;
                  }))))
      .Times(1);
  EXPECT_CALL(socket, set_option(boost::asio::socket_base::broadcast(true)))
      .Times(1);

  jb::itch5::detail::setup_socket_udp_send(
      socket, jb::itch5::udp_sender_config()
                  .address("192.168.1.7")
                  .port(50000)
                  .broadcast(true)
                  .hops(10));
}
