#include <jb/itch5/make_socket_udp_send.hpp>

#include <skye/mock_function.hpp>
#include <boost/asio.hpp>
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
namespace {

/// Shorthand for this program only
namespace ip = boost::asio::ip;
using socket_base = ::boost::asio::socket_base;


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

  void set_option(ip::multicast::hops const& o) {
    set_option_multicast_hops(o);
  }
  void set_option(ip::multicast::outbound_interface const& o) {
    set_option_unbound_interface(o);
  }
  void set_option(ip::unicast::hops const& o) {
    set_option_unicast_hops(o);
  }
  void set_option(socket_base::broadcast const& o) {
    set_option_broadcast(o);
  }
  void set_option(socket_base::debug const& o) {
    set_option_debug(o);
  }
  void set_option(socket_base::do_not_route const& o) {
    set_option_do_not_route(o);
  }
  void set_option(socket_base::linger const& o) {
    set_option_linger(o);
  }
  void set_option(socket_base::receive_buffer_size const& o) {
    set_option_receive_buffer_size(o);
  }
  void set_option(socket_base::receive_low_watermark const& o) {
    set_option_receive_low_watermark(o);
  }
  void set_option(socket_base::send_buffer_size const& o) {
    set_option_send_buffer_size(o);
  }
  void set_option(socket_base::send_low_watermark const& o) {
    set_option_send_low_watermark(o);
  }
  
  skye::mock_function<void(boost::asio::io_service& io)> constructor;

  skye::mock_function<void(ip::udp::socket::protocol_type)> open;
  skye::mock_function<void(ip::udp::socket::endpoint_type)> bind;
  skye::mock_function<void(ip::udp::socket::reuse_address)>
      set_option_reuse_address;
  skye::mock_function<void(ip::multicast::join_group)>
      set_option_join_group;
  skye::mock_function<void(ip::multicast::enable_loopback)>
      set_option_enable_loopback;

  skye::mock_function<void(ip::multicast::hops)> set_option_multicast_hops;
  skye::mock_function<void(ip::multicast::outbound_interface)>
      set_option_unbound_interface;
  skye::mock_function<void(ip::unicast::hops)> set_option_unicast_hops;
  skye::mock_function<void(socket_base::broadcast)> set_option_broadcast;
  skye::mock_function<void(socket_base::debug)> set_option_debug;
  skye::mock_function<void(socket_base::do_not_route)> set_option_do_not_route;
  skye::mock_function<void(socket_base::linger)> set_option_linger;
  skye::mock_function<void(socket_base::receive_buffer_size)>
      set_option_receive_buffer_size;
  skye::mock_function<void(socket_base::receive_low_watermark)>
      set_option_receive_low_watermark;
  skye::mock_function<void(socket_base::send_buffer_size)>
      set_option_send_buffer_size;
  skye::mock_function<void(socket_base::send_low_watermark)>
      set_option_send_low_watermark;
};

} // anonymous namespace

BOOST_AUTO_TEST_CASE(itch5_make_socket_udp_send_basic) {
  boost::asio::io_service io;

  // A simple unicast socket on the default interface ...
  mock_socket socket = jb::itch5::make_socket_udp_send<mock_socket>(
      io, jb::itch5::udp_sender_config().address("::1").port(50000));
  socket.open.check_called().once();
  socket.bind.check_called().once();
  socket.set_option_join_group.check_called().never();
  socket.set_option_enable_loopback.check_called().never();
}

#if 0
BOOST_AUTO_TEST_CASE(itch5_make_socket_udp_send_multicast_ipv4) {
  boost::asio::io_service io;

  // Create a IPv4 multicast socket on the default interface ...
  mock_socket socket = jb::itch5::make_socket_udp_send<mock_socket>(
      io, jb::itch5::udp_sender_config().address("239.128.1.1").port(50000));
  socket.open.check_called().once();
  socket.bind.check_called().once().with(
      boost::asio::ip::udp::endpoint(boost::asio::ip::address_v4(), 50000));
  socket.set_option_join_group.check_called().once();
  socket.set_option_enable_loopback.check_called().once();
}

BOOST_AUTO_TEST_CASE(itch5_make_socket_udp_send_multicast_ipv6) {
  boost::asio::io_service io;

  // Create a IPv6 multicast socket on the default interface ...
  mock_socket socket = jb::itch5::make_socket_udp_send<mock_socket>(
      io, jb::itch5::udp_sender_config().address("ff05::").port(50000));
  socket.open.check_called().once();
  socket.bind.check_called().once().with(
      boost::asio::ip::udp::endpoint(boost::asio::ip::address_v6(), 50000));
  socket.set_option_join_group.check_called().once();
  socket.set_option_enable_loopback.check_called().once();
}

BOOST_AUTO_TEST_CASE(itch5_make_socket_udp_send_listen_address) {
  boost::asio::io_service io;

  // Create a multicast socket on an specific interface ...
  char const* interface = "2001:db8:ca2:2::1";
  mock_socket socket = jb::itch5::make_socket_udp_send<mock_socket>(
      io, jb::itch5::udp_sender_config()
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
#endif /* 0 */
