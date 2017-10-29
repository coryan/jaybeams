#ifndef jb_itch5_testing_mock_udp_socket_hpp
#define jb_itch5_testing_mock_udp_socket_hpp

#include <boost/asio.hpp>
#include <gmock/gmock.h>

namespace jb {
namespace itch5 {
namespace testing {

/// A Mock Object for the socket class
struct mock_udp_socket {
  /// Constructor, create a mock instance
  mock_udp_socket() {
  }
  explicit mock_udp_socket(boost::asio::io_service& io) {
  }

  MOCK_METHOD1(open, void(boost::asio::ip::udp::socket::protocol_type));
  MOCK_METHOD1(bind, void(boost::asio::ip::udp::socket::endpoint_type));
  MOCK_METHOD1(set_option, void(boost::asio::ip::multicast::join_group const&));
  MOCK_METHOD1(set_option, void(boost::asio::ip::multicast::hops const&));
  MOCK_METHOD1(set_option, void(boost::asio::ip::unicast::hops const&));
  MOCK_METHOD1(set_option, void(boost::asio::socket_base::broadcast const&));
  MOCK_METHOD1(set_option, void(boost::asio::socket_base::debug const&));
  MOCK_METHOD1(set_option, void(boost::asio::socket_base::do_not_route const&));
  MOCK_METHOD1(set_option, void(boost::asio::socket_base::linger const&));
  MOCK_METHOD1(
      set_option, void(boost::asio::ip::multicast::enable_loopback const&));
  MOCK_METHOD1(
      set_option, void(boost::asio::ip::multicast::outbound_interface const&));
  MOCK_METHOD1(
      set_option, void(boost::asio::ip::udp::socket::reuse_address const&));
  MOCK_METHOD1(
      set_option, void(boost::asio::socket_base::receive_buffer_size const&));
  MOCK_METHOD1(
      set_option, void(boost::asio::socket_base::receive_low_watermark const&));
  MOCK_METHOD1(
      set_option, void(boost::asio::socket_base::send_buffer_size const&));
  MOCK_METHOD1(
      set_option, void(boost::asio::socket_base::send_low_watermark const&));
};

} // namespace testing
} // namespace itch5
} // namespace jb

#endif // jb_itch5_testing_mock_udp_socket_hpp
