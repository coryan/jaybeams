#ifndef jb_itch5_mold_udp_channel_hpp
#define jb_itch5_mold_udp_channel_hpp

#include <boost/asio/io_service.hpp>
#include <boost/asio/ip/udp.hpp>

#include <chrono>
#include <functional>

namespace jb {
namespace itch5 {

class mold_udp_channel {
 public:
  typedef std::function<void(
      std::chrono::steady_clock::time_point, std::uint64_t, std::size_t,
      char const*, std::size_t)> buffer_handler;
  
  mold_udp_channel(
      buffer_handler handler,
      boost::asio::io_service& io,
      std::string const& listen_address,
      int multicast_port,
      std::string const& multicast_group);

 private:
  void restart_async_receive_from();
  void handle_received(
      boost::system::error_code const& ec, size_t bytes_received);

 private:
  buffer_handler handler_;
  boost::asio::ip::udp::socket socket_;
  std::uint64_t expected_sequence_number_;
  std::size_t message_offset_;

  static std::size_t const buflen = 1<<16;
  char buffer_[buflen];
  boost::asio::ip::udp::endpoint sender_endpoint_;
};

} // namespace itch5
} // namespace jb

#endif // jb_itch5_mold_udp_channel_hpp
