#include "jb/itch5/udp_receiver_config.hpp"
#include <jb/usage.hpp>

#include <boost/asio/ip/udp.hpp>
#include <sstream>

namespace jb {
namespace itch5 {

udp_receiver_config::udp_receiver_config()
    : address(
          desc("address").help(
              "The destination address of the packets to receive."
              "  When receiving unicast messages this must be one of the "
              "addresses of the host, and local_address must be empty."
              "  When receiving multicast messages this is the multicast "
              "group of the messages to receive."),
          this, "")
    , port(
          desc("port").help("The UDP port of the packets to receive."), this, 0)
    , local_address(
          desc("local-address")
              .help(
                  "The local address of the receive socket."
                  "  If the value of --address is a unicast address this "
                  "must be empty."
                  "  If the value of --address is a multicast address this "
                  "can be one of the local addresses of the host, in which "
                  "cast that binds the socket to a specific interface to "
                  "receive the multicast messages."
                  "  If the value of --address is a multicast address, and "
                  "this option is empty, the the system picks the right "
                  "ADDRANY to receive the messages."),
          this, "") {
}

void udp_receiver_config::validate() const {
  udp_config_common::validate();
  if (address() == "" and port() == 0) {
    return;
  }
  auto a = boost::asio::ip::address::from_string(address());
  if (not a.is_multicast() and local_address() != "") {
    std::ostringstream os;
    os << "Invalid configuration for udp_receiver.  --address (" << address()
       << ") is a unicast address, and --local-address is not "
       << "empty (" << local_address() << ")";
    throw jb::usage(os.str(), 1);
  }
}

} // namespace itch5
} // namespace jb
