#include <jb/itch5/make_socket_udp_send.hpp>

#include <jb/itch5/testing/mock_udp_socket.hpp>
#include <boost/test/unit_test.hpp>

/**
 * @test Verify that jb::itch5::make_socket_udp_send compiles.
 */
BOOST_AUTO_TEST_CASE(itch5_make_socket_udp_send_compile) {
  boost::asio::io_service io;
  jb::itch5::udp_sender_config cfg;
  auto socket =
      jb::itch5::make_socket_udp_send(io, cfg.address("127.0.0.1").port(40000));
  BOOST_CHECK(socket.is_open());
}

/**
 * Types used in testing of jb::itch5::make_socket_udp_send
 */
namespace {} // anonymous namespace

using jb::itch5::testing::mock_udp_socket;

BOOST_AUTO_TEST_CASE(itch5_make_socket_udp_send_basic) {
  boost::asio::io_service io;

  // A simple unicast socket on the default interface ...
  mock_udp_socket socket = jb::itch5::make_socket_udp_send<mock_udp_socket>(
      io, jb::itch5::udp_sender_config().address("::1").port(50000));
  socket.open.check_called().once();
  socket.bind.check_called().once();
  socket.set_option_join_group.check_called().never();
  socket.set_option_enable_loopback.check_called().never();
}

BOOST_AUTO_TEST_CASE(itch5_make_socket_udp_send_multicast_ipv4) {
  boost::asio::io_service io;

  // Create a IPv4 multicast socket on the default interface ...
  mock_udp_socket socket = jb::itch5::make_socket_udp_send<mock_udp_socket>(
      io, jb::itch5::udp_sender_config().address("239.128.1.1").port(50000));
  socket.open.check_called().once();
  socket.bind.check_called().once().with(
      boost::asio::ip::udp::endpoint(boost::asio::ip::address_v4(), 0));
  socket.set_option_enable_loopback.check_called().once();
}

BOOST_AUTO_TEST_CASE(itch5_make_socket_udp_send_multicast_ipv6) {
  boost::asio::io_service io;

  // Create a IPv6 multicast socket on the default interface ...
  mock_udp_socket socket = jb::itch5::make_socket_udp_send<mock_udp_socket>(
      io, jb::itch5::udp_sender_config().address("ff05::").port(50000));
  socket.open.check_called().once();
  socket.bind.check_called().once().with(
      boost::asio::ip::udp::endpoint(boost::asio::ip::address_v6(), 0));
  socket.set_option_enable_loopback.check_called().once();
}

BOOST_AUTO_TEST_CASE(itch5_make_socket_udp_send_multicast_options) {
  boost::asio::io_service io;

  // Create a multicast socket on an specific interface ...
  mock_udp_socket v6s = jb::itch5::make_socket_udp_send<mock_udp_socket>(
      io, jb::itch5::udp_sender_config()
              .address("ff05::")
              .port(50000)
              .enable_loopback(true)
              .hops(10)
              .outbound_interface("2"));
  v6s.open.check_called().once();
  v6s.bind.check_called().once().with(
      boost::asio::ip::udp::endpoint(boost::asio::ip::address_v6(), 0));
  v6s.set_option_enable_loopback.check_called().once();
  v6s.set_option_multicast_hops.check_called().once();

  mock_udp_socket v4s = jb::itch5::make_socket_udp_send<mock_udp_socket>(
      io, jb::itch5::udp_sender_config()
              .address("239.128.1.1")
              .port(50000)
              .enable_loopback(true)
              .hops(-1)
              .outbound_interface("127.0.0.1"));
  v4s.open.check_called().once();
  v4s.bind.check_called().once().with(
      boost::asio::ip::udp::endpoint(boost::asio::ip::address_v4(), 0));
  v4s.set_option_enable_loopback.check_called().once();
  v4s.set_option_multicast_hops.check_called().never();
}

BOOST_AUTO_TEST_CASE(itch5_make_socket_udp_send_multicast_options_errors) {
  boost::asio::io_service io;

  // Create a multicast socket on an specific interface ...
  BOOST_CHECK_THROW(
      jb::itch5::make_socket_udp_send<mock_udp_socket>(
          io, jb::itch5::udp_sender_config()
                  .address("ff05::")
                  .port(50000)
                  .enable_loopback(true)
                  .hops(10)
                  .outbound_interface("abcd")),
      std::runtime_error);
}

BOOST_AUTO_TEST_CASE(itch5_make_socket_udp_send_unicast_options) {
  boost::asio::io_service io;

  // Create a multicast socket on an specific interface ...
  auto socket = jb::itch5::make_socket_udp_send<mock_udp_socket>(
      io, jb::itch5::udp_sender_config()
              .address("192.168.1.7")
              .port(50000)
              .broadcast(true)
              .hops(10));
  socket.open.check_called().once();
  socket.bind.check_called().once().with(
      boost::asio::ip::udp::endpoint(boost::asio::ip::address_v4(), 0));
  socket.set_option_unicast_hops.check_called().once();
  socket.set_option_broadcast.check_called().once();
}
