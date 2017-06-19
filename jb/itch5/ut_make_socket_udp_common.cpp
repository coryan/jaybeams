#include <jb/itch5/make_socket_udp_common.hpp>

#include <jb/itch5/testing/mock_udp_socket.hpp>
#include <boost/test/unit_test.hpp>

using jb::itch5::testing::mock_udp_socket;

BOOST_AUTO_TEST_CASE(itch5_make_socket_udp_common_basic) {
  boost::asio::io_service io;

  // Create a mock socket and configure it with different options ...
  mock_udp_socket socket(io);
  jb::itch5::make_socket_udp_common(socket, jb::itch5::udp_config_common());
  socket.set_option_debug.check_called().never();
  socket.set_option_do_not_route.check_called().once();
  socket.set_option_linger.check_called().never();
  socket.set_option_receive_buffer_size.check_called().never();
  socket.set_option_receive_low_watermark.check_called().never();
  socket.set_option_send_buffer_size.check_called().never();
  socket.set_option_send_low_watermark.check_called().never();
}

BOOST_AUTO_TEST_CASE(itch5_make_socket_udp_common_debug) {
  boost::asio::io_service io;

  // Create a mock socket and configure it with different options ...
  mock_udp_socket socket(io);
  jb::itch5::make_socket_udp_common(
      socket, jb::itch5::udp_config_common().debug(true));
  socket.set_option_debug.check_called().once();
}

BOOST_AUTO_TEST_CASE(itch5_make_socket_udp_common_linger) {
  boost::asio::io_service io;

  // Create a mock socket and configure it with different options ...
  mock_udp_socket socket(io);
  jb::itch5::make_socket_udp_common(
      socket, jb::itch5::udp_config_common().linger(true).linger_seconds(30));
  socket.set_option_linger.check_called().once();
}

BOOST_AUTO_TEST_CASE(itch5_make_socket_udp_common_rcvbuf) {
  boost::asio::io_service io;

  // Create a mock socket and configure it with different options ...
  mock_udp_socket socket(io);
  jb::itch5::make_socket_udp_common(
      socket, jb::itch5::udp_config_common().receive_buffer_size(8192));
  socket.set_option_receive_buffer_size.check_called().once();
}

BOOST_AUTO_TEST_CASE(itch5_make_socket_udp_common_sndbuf) {
  boost::asio::io_service io;

  // Create a mock socket and configure it with different options ...
  mock_udp_socket socket(io);
  jb::itch5::make_socket_udp_common(
      socket, jb::itch5::udp_config_common().send_buffer_size(8192));
  socket.set_option_send_buffer_size.check_called().once();
}

BOOST_AUTO_TEST_CASE(itch5_make_socket_udp_common_rcvlow) {
  boost::asio::io_service io;

  // Create a mock socket and configure it with different options ...
  mock_udp_socket socket(io);
  jb::itch5::make_socket_udp_common(
      socket, jb::itch5::udp_config_common().receive_low_watermark(8192));
  socket.set_option_receive_low_watermark.check_called().once();
}

BOOST_AUTO_TEST_CASE(itch5_make_socket_udp_common_sndlow) {
  boost::asio::io_service io;

  // Create a mock socket and configure it with different options ...
  mock_udp_socket socket(io);
  jb::itch5::make_socket_udp_common(
      socket, jb::itch5::udp_config_common().send_low_watermark(8192));
  socket.set_option_send_low_watermark.check_called().once();
}
