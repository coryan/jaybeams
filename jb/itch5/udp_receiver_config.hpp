#ifndef jb_itch5_udp_receiver_config_hpp
#define jb_itch5_udp_receiver_config_hpp

#include <jb/itch5/udp_config_common.hpp>

namespace jb {
namespace itch5 {

/**
 * A configuration object for UDP receivers.
 *
 * The @a receive_address attribute the destination address for the
 * messages that we want to receive.  For unicast messages that must
 * be one of the addresses of the host.  For multicast messages that
 * is simply the multicast group to receive, in this case, @a
 * listen_address the local address for the socket.  If @a
 * listen_address is the empty string, the local address is guessed
 * based on the value of @a receive_address: (a) for IPv4 multicast
 * groups simply use 0.0.0.0, (b) for IPv6 multicast groups simply use
 * ::1.  It is an error to configure @a receive_address as a unicast
 * address and also set @a listen_address.
 */
class udp_receiver_config : public udp_config_common {
public:
  udp_receiver_config();
  config_object_constructors(udp_receiver_config);

  void validate() const override;

  jb::config_attribute<udp_receiver_config, std::string> address;
  jb::config_attribute<udp_receiver_config, int> port;
  jb::config_attribute<udp_receiver_config, std::string> local_address;
};

} // namespace itch5
} // namespace jb

#endif // jb_itch5_udp_receiver_config_hpp
