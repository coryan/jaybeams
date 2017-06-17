#include "jb/itch5/udp_receiver_config.hpp"

namespace jb {
namespace itch5 {

udp_receiver_config::udp_receiver_config()
    : port(
          desc("port").help("The UDP port of the packets to receive."), this, 0)
    , listen_address(
          desc("listen-address")
              .help(
                  "The address where to receive UDP packets."
                  "  Typically this is the address of one of the network "
                  "adapters on the host, though it can be ::1 or 127.0.0.1 "
                  "for development and testing."),
          this, "")
    , receive_address(
          desc("receive-address")
              .help(
                  "If not empty, the destination address of the UDP packets "
                  "to receive."
                  "  When using multicast this is the multicast group to "
                  "receive."
                  " The listen-address and receive-address parameters must "
                  "use compatible protocol versions (IPv4 vs. IPv6)."),
          this, "") {
}

} // namespace itch5
} // namespace jb
