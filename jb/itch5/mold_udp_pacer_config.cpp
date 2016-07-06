#include <jb/itch5/mold_udp_pacer_config.hpp>

#include <chrono>
#include <sstream>
#include <stdexcept>

namespace jb {
namespace itch5 {
/// Default the default values for ITCH-5.x configuation.
namespace defaults {

#ifndef JB_ITCH5_DEFAULTS_maximum_delay_microseconds
#define JB_ITCH5_DEFAULTS_maximum_delay_microseconds 100
#endif // JB_ITCH5_DEFAULTS_maximum_delay_microseconds

/*
 * Getting a good default for the MTU is hard.  Most Ethernet networks
 * can tolerate MTUs of 1500 octets.  Including the minimum IPv4
 * header (20 octets, 40 for IPv6), and the minimum UDP header (8) the
 * usual recommendation is 1500 - 20 - 8 = 1472.  But this is easily
 * wrong as the UDP header can be as large as 60 octets, and the MTU
 * may be significantly smaller (or larger!).
 *
 * The only guarantee is that all hosts should be prepared to accept
 * datagrams of up to 576 octets: 
 *   https://tools.ietf.org/html/rfc791
 * effectively, 576 is the "minimum" value for the MTU.
 *
 * So the most conservative approach would be to limit our payload to
 * 576 minus the largest headers that could exist.  For IPv4 the
 * maximum header is 60 bytes, so the limit would be:
 *
 *   ConservativeMTU(IPv4) = 576 - 60 - 8 = 508
 *
 * for IPv6 the situation is a lot more complicated, as far as I can
 * tell there is no limit for the IPv6 header.
 */
#ifndef JB_ITCH5_DEFAULTS_maximum_transmission_unit
#define JB_ITCH5_DEFAULTS_maximum_transmission_unit 508
#endif // JB_ITCH5_DEFAULTS_maximum_transmission_unit

int maximum_delay_microseconds = JB_ITCH5_DEFAULTS_maximum_delay_microseconds;
int maximum_transmission_unit = JB_ITCH5_DEFAULTS_maximum_transmission_unit;

} // namespace defaults

mold_udp_pacer_config::mold_udp_pacer_config()
    : maximum_delay_microseconds(
        desc("maximum-delay-microseconds").help(
            "Maximum time a MoldUDP packet is delayed before sending it."),
        this, defaults::maximum_delay_microseconds)
    , maximum_transmission_unit(
        desc("maximum-transmission-unit").help(
            "Maximum MoldUDP message to be sent in a single UDP message. "
            "The default value is extremely conservative. "
            "If your Ethernet network is configured for an MTU of 1500, "
            "use 1432 for this value.  Beware of VLANs and other details "
            "that may consume your available bytes."),
        this, defaults::maximum_transmission_unit)
{}

void mold_udp_pacer_config::validate() const {
  // The UDP payload length is encoded in a 16-bit number, so no
  // matter how clever your network the payload cannot exceed 1<<16 -
  // 1.  Well, with jumbograms you could go as big as 1<<32 - 1, but
  // that is completely wrong for the type of data that MoldUDP64
  // carries ...
  int const max_udp_payload = (1<<16) - 1;
  if (maximum_transmission_unit() < 0
      or maximum_transmission_unit() >= max_udp_payload) {
    std::ostringstream os;
    os << "--maximum-transimission-unit must be in the [0,"
       << max_udp_payload << "] range, value="
       << maximum_transmission_unit();
    throw jb::usage{os.str(), 1};
  }

  // ... a delay for over a day makes no sense for this type of data ...
  using namespace std::chrono;
  auto const day_in_usecs = duration_cast<microseconds>(hours(24));
  if (maximum_delay_microseconds() < 0
      or maximum_delay_microseconds() >= day_in_usecs.count()) {
    std::ostringstream os;
    os << "--maximum-delay-microseconds must be in the [0,"
       << day_in_usecs.count() << " (24 hours)] range, value="
       << maximum_delay_microseconds();
    throw jb::usage{os.str(), 1};
  }
}

} // namespace itch5
} // namespace jb
