#include <jb/itch5/make_socket_udp_recv.hpp>

#include <skye/mock_function.hpp>
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
namespace {

/// A Mock Object for the socket class
struct mock_socket {
  explicit mock_socket(boost::asio::io_service& io) {
    constructor(io);
  }

  void set_option(boost::asio::ip::udp::socket::reuse_address const& o) {
    set_option_reuse_address(o);
  }

  void set_option(boost::asio::ip::multicast::join_group const& o) {
    set_option_join_group(o);
  }

  void set_option(boost::asio::ip::multicast::enable_loopback const& o) {
    set_option_enable_loopback(o);
  }

  skye::mock_function<void(boost::asio::io_service& io)> constructor;
  skye::mock_function<void(boost::asio::ip::udp::socket::protocol_type)> open;
  skye::mock_function<void(boost::asio::ip::udp::socket::endpoint_type)> bind;
  skye::mock_function<void(boost::asio::ip::udp::socket::reuse_address)>
      set_option_reuse_address;
  skye::mock_function<void(boost::asio::ip::multicast::join_group)>
      set_option_join_group;
  skye::mock_function<void(boost::asio::ip::multicast::enable_loopback)>
      set_option_enable_loopback;
};

} // anonymous namespace

BOOST_AUTO_TEST_CASE(itch5_make_socket_udp_recv_basic) {
  boost::asio::io_service io;

  // A simple unicast socket on the default interface ...
  mock_socket socket = jb::itch5::make_socket_udp_recv<mock_socket>(
      io, jb::itch5::udp_receiver_config().address("::1").port(50000));
  socket.open.check_called().once();
  socket.bind.check_called().once();
  socket.set_option_join_group.check_called().never();
  socket.set_option_enable_loopback.check_called().never();
}

BOOST_AUTO_TEST_CASE(itch5_make_socket_udp_recv_multicast_ipv4) {
  boost::asio::io_service io;

  // Create a IPv4 multicast socket on the default interface ...
  mock_socket socket = jb::itch5::make_socket_udp_recv<mock_socket>(
      io, jb::itch5::udp_receiver_config().address("239.128.1.1").port(50000));
  socket.open.check_called().once();
  socket.bind.check_called().once().with(
      boost::asio::ip::udp::endpoint(boost::asio::ip::address_v4(), 50000));
  socket.set_option_join_group.check_called().once();
  socket.set_option_enable_loopback.check_called().once();
}

BOOST_AUTO_TEST_CASE(itch5_make_socket_udp_recv_multicast_ipv6) {
  boost::asio::io_service io;

  // Create a IPv6 multicast socket on the default interface ...
  mock_socket socket = jb::itch5::make_socket_udp_recv<mock_socket>(
      io, jb::itch5::udp_receiver_config().address("ff05::").port(50000));
  socket.open.check_called().once();
  socket.bind.check_called().once().with(
      boost::asio::ip::udp::endpoint(boost::asio::ip::address_v6(), 50000));
  socket.set_option_join_group.check_called().once();
  socket.set_option_enable_loopback.check_called().once();
}

BOOST_AUTO_TEST_CASE(itch5_make_socket_udp_recv_listen_address) {
  boost::asio::io_service io;

  // Create a multicast socket on an specific interface ...
  char const* interface = "2001:db8:ca2:2::1";
  mock_socket socket = jb::itch5::make_socket_udp_recv<mock_socket>(
      io, jb::itch5::udp_receiver_config()
              .address("ff05::")
              .port(50000)
              .local_address(interface));
  socket.open.check_called().once();
  socket.bind.check_called().once().with(
      boost::asio::ip::udp::endpoint(
          boost::asio::ip::address::from_string(interface), 50000));
  socket.set_option_join_group.check_called().once();
  socket.set_option_enable_loopback.check_called().once();
}
