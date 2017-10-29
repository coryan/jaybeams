#include <jb/itch5/make_socket_udp_recv.hpp>

#include <jb/itch5/testing/mock_udp_socket.hpp>
#include <boost/test/unit_test.hpp>

/**
 * @test Verify that jb::itch5::make_socket_udp_recv compiles.
 */
BOOST_AUTO_TEST_CASE(itch5_make_socket_udp_recv_compile) {
  boost::asio::io_service io;
  jb::itch5::udp_receiver_config cfg;
  auto socket = jb::itch5::make_socket_udp_recv(io, cfg.address("127.0.0.1"));
  BOOST_CHECK(socket.is_open());
}

/**
 * Types used in testing of jb::itch5::make_socket_udp_recv
 */
namespace {} // anonymous namespace

using jb::itch5::testing::mock_udp_socket;

BOOST_AUTO_TEST_CASE(itch5_make_socket_udp_recv_basic) {
  // A simple unicast socket on the default interface ...
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

  jb::itch5::detail::setup_socket_udp_recv(
      socket, jb::itch5::udp_receiver_config().address("::1").port(50000));
}

BOOST_AUTO_TEST_CASE(itch5_make_socket_udp_recv_multicast_ipv4) {
  using namespace ::testing;
  NiceMock<mock_udp_socket> socket;
  EXPECT_CALL(socket, open(_)).Times(1);
  EXPECT_CALL(
      socket, bind(boost::asio::ip::udp::endpoint(
                  boost::asio::ip::address_v4(), 50000)))
      .Times(1);
  EXPECT_CALL(
      socket, set_option(An<boost::asio::ip::multicast::join_group const&>()))
      .Times(1);
  EXPECT_CALL(
      socket,
      set_option(An<boost::asio::ip::multicast::enable_loopback const&>()))
      .Times(1);

  // Create a IPv4 multicast socket on the default interface ...
  jb::itch5::detail::setup_socket_udp_recv(
      socket,
      jb::itch5::udp_receiver_config().address("239.128.1.1").port(50000));
}

BOOST_AUTO_TEST_CASE(itch5_make_socket_udp_recv_multicast_ipv6) {
  using namespace ::testing;
  NiceMock<mock_udp_socket> socket;
  EXPECT_CALL(socket, open(_)).Times(1);
  EXPECT_CALL(
      socket, bind(boost::asio::ip::udp::endpoint(
                  boost::asio::ip::address_v6(), 50000)))
      .Times(1);
  EXPECT_CALL(
      socket, set_option(An<boost::asio::ip::multicast::join_group const&>()))
      .Times(1);
  EXPECT_CALL(
      socket,
      set_option(An<boost::asio::ip::multicast::enable_loopback const&>()))
      .Times(1);

  // Create a IPv6 multicast socket on the default interface ...
  jb::itch5::detail::setup_socket_udp_recv(
      socket, jb::itch5::udp_receiver_config().address("ff05::").port(50000));
}

BOOST_AUTO_TEST_CASE(itch5_make_socket_udp_recv_listen_address) {
  using namespace ::testing;
  NiceMock<mock_udp_socket> socket;
  char const* interface = "2001:db8:ca2:2::1";
  EXPECT_CALL(socket, open(_)).Times(1);
  EXPECT_CALL(
      socket, bind(boost::asio::ip::udp::endpoint(
                  boost::asio::ip::address::from_string(interface), 50000)))
      .Times(1);
  EXPECT_CALL(
      socket, set_option(An<boost::asio::ip::multicast::join_group const&>()))
      .Times(1);
  EXPECT_CALL(
      socket,
      set_option(An<boost::asio::ip::multicast::enable_loopback const&>()))
      .Times(1);

  // Create a multicast socket on an specific interface ...
  jb::itch5::detail::setup_socket_udp_recv(
      socket,
      jb::itch5::udp_receiver_config()
          .address("ff05::")
          .port(50000)
          .local_address(interface));
}
