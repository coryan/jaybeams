#ifndef jb_itch5_testing_mock_udp_socket_hpp
#define jb_itch5_testing_mock_udp_socket_hpp

#include <skye/mock_function.hpp>
#include <boost/asio.hpp>

namespace jb {
namespace itch5 {
namespace testing {

/// A Mock Object for the socket class
struct mock_udp_socket {
  /// Constructor, crate a mock instance
  explicit mock_udp_socket(boost::asio::io_service& io) {
    constructor(io);
  }

  //@{
  /**
   * Interface implementation, forward to mock funtions.
   */
  void set_option(boost::asio::ip::udp::socket::reuse_address const& o) {
    set_option_reuse_address(o);
  }
  void set_option(boost::asio::ip::multicast::join_group const& o) {
    set_option_join_group(o);
  }
  void set_option(boost::asio::ip::multicast::enable_loopback const& o) {
    set_option_enable_loopback(o);
  }
  void set_option(boost::asio::ip::multicast::hops const& o) {
    set_option_multicast_hops(o);
  }
  void set_option(boost::asio::ip::multicast::outbound_interface const& o) {
    set_option_unbound_interface(o);
  }
  void set_option(boost::asio::ip::unicast::hops const& o) {
    set_option_unicast_hops(o);
  }
  void set_option(boost::asio::socket_base::broadcast const& o) {
    set_option_broadcast(o);
  }
  void set_option(boost::asio::socket_base::debug const& o) {
    set_option_debug(o);
  }
  void set_option(boost::asio::socket_base::do_not_route const& o) {
    set_option_do_not_route(o);
  }
  void set_option(boost::asio::socket_base::linger const& o) {
    set_option_linger(o);
  }
  void set_option(boost::asio::socket_base::receive_buffer_size const& o) {
    set_option_receive_buffer_size(o);
  }
  void set_option(boost::asio::socket_base::receive_low_watermark const& o) {
    set_option_receive_low_watermark(o);
  }
  void set_option(boost::asio::socket_base::send_buffer_size const& o) {
    set_option_send_buffer_size(o);
  }
  void set_option(boost::asio::socket_base::send_low_watermark const& o) {
    set_option_send_low_watermark(o);
  }
  //@}

  //@{
  /**
   * Mock functions
   */
  skye::mock_function<void(boost::asio::io_service& io)> constructor;
  skye::mock_function<void(boost::asio::ip::udp::socket::protocol_type)> open;
  skye::mock_function<void(boost::asio::ip::udp::socket::endpoint_type)> bind;
  skye::mock_function<void(boost::asio::ip::udp::socket::reuse_address)>
      set_option_reuse_address;
  skye::mock_function<void(boost::asio::ip::multicast::join_group)>
      set_option_join_group;
  skye::mock_function<void(boost::asio::ip::multicast::enable_loopback)>
      set_option_enable_loopback;

  skye::mock_function<void(boost::asio::ip::multicast::hops)>
      set_option_multicast_hops;
  skye::mock_function<void(boost::asio::ip::multicast::outbound_interface)>
      set_option_unbound_interface;
  skye::mock_function<void(boost::asio::ip::unicast::hops)>
      set_option_unicast_hops;
  skye::mock_function<void(boost::asio::socket_base::broadcast)>
      set_option_broadcast;
  skye::mock_function<void(boost::asio::socket_base::debug)> set_option_debug;
  skye::mock_function<void(boost::asio::socket_base::do_not_route)>
      set_option_do_not_route;
  skye::mock_function<void(boost::asio::socket_base::linger)> set_option_linger;
  skye::mock_function<void(boost::asio::socket_base::receive_buffer_size)>
      set_option_receive_buffer_size;
  skye::mock_function<void(boost::asio::socket_base::receive_low_watermark)>
      set_option_receive_low_watermark;
  skye::mock_function<void(boost::asio::socket_base::send_buffer_size)>
      set_option_send_buffer_size;
  skye::mock_function<void(boost::asio::socket_base::send_low_watermark)>
      set_option_send_low_watermark;
  //@}
};

} // namespace testing
} // namespace itch5
} // namespace jb

#endif // jb_itch5_testing_mock_udp_socket_hpp
