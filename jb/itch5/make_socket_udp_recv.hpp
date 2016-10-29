#ifndef jb_itch5_make_socket_udp_recv_hpp
#define jb_itch5_make_socket_udp_recv_hpp

#include <boost/asio/ip/udp.hpp>
#include <boost/asio/ip/multicast.hpp>

#include <string>

namespace jb {
namespace itch5 {

/**
 * Create a socket given the configuration parameters.
 *
 * This is a function to create (open) sockets to receive UDP
 * messages, either unicast or multicast and either IPv4 or IPv6.
 *
 * @tparam socket_t the type of socket to create.  The main reason to
 * change this type is for dependency injection during testing.
 *
 * @param io the Boost.ASIO io_service object that the socket is
 * managed by.
 *
 * @param receive_address the address to listen for messages on.  Can
 * be a unicast or multicast address.
 *
 * @param port the port for the socket
 *
 * @param listen_address the local address for the socket.  If this is
 * the empty string, the local address is guessed based on the
 * receive_address: (a) for IPv4 multicast groups simply use 0.0.0.0,
 * (b) for IPv6 multicast groups simply use ::1, (c) otherwise assume
 * the receive_address is a unicast address and try to use it as the
 * local address.
 *
 */
template<class socket_t = boost::asio::ip::udp::socket>
socket_t make_socket_udp_recv(
    boost::asio::io_service& io, std::string const& receive_address, int port,
    std::string const& listen_address) {
  auto r_address = boost::asio::ip::address::from_string(receive_address);

  boost::asio::ip::address local_address;

  // Automatically configure the best listening address ...
  if (listen_address != "") {
    // ... the user specified a listening address, use that ...
    local_address = boost::asio::ip::address::from_string(listen_address);
  } else if (r_address.is_multicast()) {
    // ... pick a default based on the protocol for the listening
    // group ...
    if (r_address.is_v6()) {
      local_address = boost::asio::ip::address_v6();
    } else {
      local_address = boost::asio::ip::address_v4();
    }
  } else {
    // ... whatever the receive address is, it must be a unicast
    // address, use it as the local address of the socket.  Notice
    // that it can be ADDRANY, such as ::1 ...
    local_address = r_address;
  }

  // ... the rest if fairly mechanical stuff ..
  boost::asio::ip::udp::endpoint endpoint(local_address, port);
  boost::asio::ip::udp::socket socket(io);
  socket.open(endpoint.protocol());
  socket.set_option(boost::asio::ip::udp::socket::reuse_address(true));
  socket.bind(endpoint);
  if (r_address.is_multicast()) {
    socket.set_option(
        boost::asio::ip::multicast::join_group(r_address));
    socket.set_option(boost::asio::ip::multicast::enable_loopback(true));
  }

  return socket;
}

} // namespace itch5
} // namespace jb

#endif // jb_itch5_make_socket_udp_recv_hpp
