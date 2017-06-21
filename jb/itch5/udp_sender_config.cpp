#include "jb/itch5/udp_sender_config.hpp"

namespace jb {
namespace itch5 {

udp_sender_config::udp_sender_config()
    : udp_config_common()
    , port(desc("port").help("Set the UDP port to send messages to."), this, 0)
    , address(
          desc("address").help("Set the UDP address to send messages to."
                               "  The address can be any combination of IPv4 "
                               "vs. IPv6 and unicast vs. multicast."),
          this, "")
    , enable_loopback(
          desc("enable-loopback")
              .help("Set the IP_MULTICAST_LOOP socket "
                    "option, allowing outgoing multicast "
                    "messages to be received by programs in "
                    "the same host."),
          this, false)
    , hops(
          desc("hops").help(
              "Set the IP_MULTICAST_TTL or IP_TTL option for the socket."
              "  By default, or if set to -1, the system default value is "
              "used."),
          this, -1)
    , outbound_interface(
          desc("outbound-interface")
              .help("Set the outbound interface for outgoing multicast "
                    "messages.  "
                    "When using IPv4, this must be the IP address of the "
                    "outgoing interface.  "
                    "When using IPv6, this must be the interface index."),
          this, "")
    , broadcast(
          desc("broadcast").help("Set the SO_BROADCAST option for the socket."),
          this, false) {
}

} // namespace itch5
} // namespace jb
