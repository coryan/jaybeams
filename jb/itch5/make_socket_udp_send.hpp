#ifndef jb_itch5_make_socket_udp_send_hpp
#define jb_itch5_make_socket_udp_send_hpp

#include <jb/itch5/make_socket_udp_common.hpp>
#include <jb/itch5/udp_sender_config.hpp>
#include <jb/strtonum.hpp>

#include <boost/asio/ip/multicast.hpp>
#include <boost/asio/ip/udp.hpp>
#include <boost/asio/ip/unicast.hpp>

namespace jb {
namespace itch5 {

namespace detail {
template <typename socket_t>
void setup_socket_udp_send(socket_t& s, udp_sender_config const& cfg) {
  // Some type aliases to shorten the code inside the function ...
  namespace ip = boost::asio::ip;
  using socket_base = boost::asio::socket_base;

  // ... this is the address we want to send to, we need to figure out
  // what is its protocol so we can then open the socket ...
  auto const r_address = ip::address::from_string(cfg.address());
  // ... depending on the address we need to pick the right ADDRANY
  // format ...
  ip::address local_address = ip::address_v4();
  if (r_address.is_v6()) {
    local_address = ip::address_v6();
  }
  // ... create a socket bound to the right ADDRANY and an ephemeral
  // port ...
  ip::udp::endpoint endpoint(local_address, 0);

  s.open(endpoint.protocol());
  s.set_option(socket_base::reuse_address(cfg.reuse_address()));
  s.bind(endpoint);

  // ... now apply all the socket options, some only make sense if the
  // destination is a multicast socket, apply those first ...
  if (r_address.is_multicast()) {
    s.set_option(ip::multicast::enable_loopback(cfg.enable_loopback()));
    if (cfg.hops() != -1) {
      s.set_option(ip::multicast::hops(ip::multicast::hops(cfg.hops())));
    }
    if (cfg.outbound_interface() != "") {
      if (r_address.is_v4()) {
        auto const local_if =
            ip::address_v4::from_string(cfg.outbound_interface());
        s.set_option(ip::multicast::outbound_interface(local_if));
      } else {
        int if_index;
        if (not jb::strtonum(cfg.outbound_interface(), if_index)) {
          std::ostringstream os;
          os << "Cannot convert outbound-interface value ("
             << cfg.outbound_interface() << ") to a IPv6 interface index";
          throw std::runtime_error(os.str());
        }
        s.set_option(ip::multicast::outbound_interface(if_index));
      }
    }
  } else {
    if (cfg.hops() != -1) {
      s.set_option(ip::unicast::hops(ip::unicast::hops(cfg.hops())));
    }
    s.set_option(socket_base::broadcast(cfg.broadcast()));
  }
}
} // namespace detail

/**
 * Create a socket to send UDP messages given the configuration parameters.
 *
 * This is a function to create (open) sockets to send UDP
 * messages, either unicast or multicast and either IPv4 or IPv6.  The
 * socket is bound to ADDRANY, and uses an ephemeral port selected by
 * the operating system.  The address family (v4 vs. v6) is selected
 * based on the destination address.
 *
 * @tparam socket_t the type of socket to create.  The main reason to
 * change this type is for dependency injection during testing.
 *
 * @param io the Boost.ASIO io_service object that the socket is
 * managed by.
 *
 * @param cfg the socket configuration.
 */
template <class socket_t = boost::asio::ip::udp::socket>
socket_t make_socket_udp_send(
    boost::asio::io_service& io, udp_sender_config const& cfg) {
  socket_t s(io);
  detail::setup_socket_udp_send(s, cfg);
  make_socket_udp_common(s, cfg);
  return s;
}

} // namespace itch5
} // namespace jb

#endif // jb_itch5_make_socket_udp_send_hpp
