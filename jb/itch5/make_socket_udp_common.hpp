#ifndef jb_itch5_make_socket_udp_common_hpp
#define jb_itch5_make_socket_udp_common_hpp

#include <jb/itch5/udp_config_common.hpp>

#include <boost/asio/ip/multicast.hpp>
#include <boost/asio/ip/udp.hpp>
#include <boost/asio/ip/unicast.hpp>

namespace jb {
namespace itch5 {

/**
 * Configure a UDP socket.
 *
 * @tparam socket_t the type of socket to create.  The main reason to
 * change this type is for dependency injection during testing.
 *
 * @param s the socket to configure.
 * @param cfg the socket configuration.
 */
template <class socket_t = boost::asio::ip::udp::socket>
void make_socket_udp_common(socket_t& s, udp_config_common const& cfg) {
  // Some type aliases to shorten the code inside the function ...
  using socket_base = boost::asio::socket_base;

  if (cfg.debug()) {
    s.set_option(socket_base::debug(cfg.debug()));
  }
  s.set_option(socket_base::do_not_route(cfg.do_not_route()));
  if (cfg.linger()) {
    s.set_option(socket_base::linger(cfg.linger(), cfg.linger_seconds()));
  }
  if (cfg.receive_buffer_size() != -1) {
    s.set_option(socket_base::receive_buffer_size(cfg.receive_buffer_size()));
  }
  if (cfg.receive_low_watermark() != -1) {
    s.set_option(
        socket_base::receive_low_watermark(cfg.receive_low_watermark()));
  }
  if (cfg.send_buffer_size() != -1) {
    s.set_option(socket_base::send_buffer_size(cfg.send_buffer_size()));
  }
  if (cfg.send_low_watermark() != -1) {
    s.set_option(socket_base::send_low_watermark(cfg.send_low_watermark()));
  }
}

} // namespace itch5
} // namespace jb

#endif // jb_itch5_make_socket_udp_common_hpp
