#ifndef jb_itch5_mold_udp_channel_hpp
#define jb_itch5_mold_udp_channel_hpp

#include <boost/asio/io_service.hpp>
#include <boost/asio/ip/udp.hpp>

#include <chrono>
#include <functional>

namespace jb {
namespace itch5 {

/**
 * Create and manage a socket to receive MoldUDP64 packets.
 *
 * This class creates a socket to receive MoldUDP64 packets, 
 * registers with the Boost.ASIO IO service to be notified of new
 * packets in the socket, and when new packets are received it breaks
 * down the packet into ITCH-5.0 messages and invokes a handler for
 * each one.
 */
class mold_udp_channel {
 public:
  /**
   * A callback function type to process any received ITCH-5.0
   * messages
   *
   * The parameters represent (in order)
   * - When was the MoldUDP64 packet containing this message received
   * - The sequence number for this particular message
   * - The offset (in bytes) from the beginning of the MoldUDP64
   * stream for this message
   * - The message, including the ITCH-5.0 headers but excluding any
   * MoldUDP64 headers.
   * - The size of the message, in bytes.
   */
  typedef std::function<void(
      std::chrono::steady_clock::time_point, std::uint64_t, std::size_t,
      char const*, std::size_t)> buffer_handler;

  /**
   * Constructor, create a socket and register for IO notifications
   *
   * @param handler the callback to invoke to process any ITCH-5.0
   * messages received
   * @param io the Boost.ASIO IO service to register with for IO
   * notifications
   * @param receive_address the IP address where the packets are
   * recevied.  This can be either IPv4 or IPv6, and either a unicast
   * or multicast address.  The class creates a socket of the right
   * type.
   * @param port the UDP port where the packets are received.
   * @param listen_address if not empty, defines the network interface
   * where the packets are received, for servers that have multiple
   * NICs.
   */
  mold_udp_channel(boost::asio::io_service& io, buffer_handler handler,
                   std::string const& receive_address, int port,
                   std::string const& listen_address);

 private:
  /**
   * Refactor code to register (and reregister) for Boost.ASIO
   * notifications
   *
   * In the Boost.ASIO framework all I/O callbacks fire only once.  We
   * must reset them to get the first I/O event, and after each one.
   * This function refactors that code into a single place.
   */
  void restart_async_receive_from();

  /// The Boost.ASIO callback for I/O events
  void handle_received(
      boost::system::error_code const& ec, size_t bytes_received);

 private:
  // The callback handler
  buffer_handler handler_;

  // A UDP socket configured as per the constructor arguments
  boost::asio::ip::udp::socket socket_;

  // The next sequence number expected from the MoldUDP64 stream
  std::uint64_t expected_sequence_number_;

  // The offset (in bytes) since the beginning of the MoldUDP64
  // stream, mostly for logging.
  std::size_t message_offset_;

  // The maximum packet length expected (UDP is limited to 2^16 bytes)
  static std::size_t const buflen = 1<<16;

  // A buffer to read the data into, when handle_received() is called
  // the data should already be in this location.
  char buffer_[buflen];

  // The UDP endpoint that sent the last received MoldUDP64 packet
  boost::asio::ip::udp::endpoint sender_endpoint_;
};

} // namespace itch5
} // namespace jb

#endif // jb_itch5_mold_udp_channel_hpp
