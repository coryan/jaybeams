#include <jb/itch5/make_socket_udp_common.hpp>

#include <jb/itch5/testing/mock_udp_socket.hpp>
#include <boost/test/unit_test.hpp>

using namespace ::testing;
using jb::itch5::testing::mock_udp_socket;

BOOST_AUTO_TEST_CASE(itch5_make_socket_udp_common_basic) {
  boost::asio::io_service io;

  // Create a mock socket and configure it with different options ...
  mock_udp_socket socket(io);
  jb::itch5::make_socket_udp_common(socket, jb::itch5::udp_config_common());
}

BOOST_AUTO_TEST_CASE(itch5_make_socket_udp_common_debug) {
  boost::asio::io_service io;

  // Create a mock socket and configure it with different options ...
  mock_udp_socket socket(io);
  EXPECT_CALL(socket, set_option(An<boost::asio::socket_base::debug const&>()));
  jb::itch5::make_socket_udp_common(
      socket, jb::itch5::udp_config_common().debug(true));
}

BOOST_AUTO_TEST_CASE(itch5_make_socket_udp_common_linger) {
  boost::asio::io_service io;

  // Create a mock socket and configure it with different options ...
  mock_udp_socket socket(io);
  EXPECT_CALL(
      socket, set_option(An<boost::asio::socket_base::linger const&>()));
  jb::itch5::make_socket_udp_common(
      socket, jb::itch5::udp_config_common().linger(true).linger_seconds(30));
}

BOOST_AUTO_TEST_CASE(itch5_make_socket_udp_common_rcvbuf) {
  boost::asio::io_service io;

  // Create a mock socket and configure it with different options ...
  mock_udp_socket socket(io);
  EXPECT_CALL(
      socket,
      set_option(An<boost::asio::socket_base::receive_buffer_size const&>()));
  jb::itch5::make_socket_udp_common(
      socket, jb::itch5::udp_config_common().receive_buffer_size(8192));
}

BOOST_AUTO_TEST_CASE(itch5_make_socket_udp_common_sndbuf) {
  boost::asio::io_service io;

  // Create a mock socket and configure it with different options ...
  mock_udp_socket socket(io);
  EXPECT_CALL(
      socket,
      set_option(An<boost::asio::socket_base::send_buffer_size const&>()));
  jb::itch5::make_socket_udp_common(
      socket, jb::itch5::udp_config_common().send_buffer_size(8192));
}

BOOST_AUTO_TEST_CASE(itch5_make_socket_udp_common_rcvlow) {
  boost::asio::io_service io;

  // Create a mock socket and configure it with different options ...
  mock_udp_socket socket(io);
  EXPECT_CALL(
      socket,
      set_option(An<boost::asio::socket_base::receive_low_watermark const&>()));
  jb::itch5::make_socket_udp_common(
      socket, jb::itch5::udp_config_common().receive_low_watermark(8192));
}

BOOST_AUTO_TEST_CASE(itch5_make_socket_udp_common_sndlow) {
  boost::asio::io_service io;

  // Create a mock socket and configure it with different options ...
  mock_udp_socket socket(io);
  EXPECT_CALL(
      socket,
      set_option(An<boost::asio::socket_base::send_low_watermark const&>()));
  jb::itch5::make_socket_udp_common(
      socket, jb::itch5::udp_config_common().send_low_watermark(8192));
}
