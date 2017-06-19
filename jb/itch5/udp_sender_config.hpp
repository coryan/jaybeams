#ifndef jb_itch5_udp_sender_config_hpp
#define jb_itch5_udp_sender_config_hpp

#include <jb/itch5/udp_config_common.hpp>

namespace jb {
namespace itch5 {

/**
 * A configuration object for UDP senders.
 */
class udp_sender_config : public jb::itch5::udp_config_common {
public:
  udp_sender_config();
  config_object_constructors(udp_sender_config);

  jb::config_attribute<udp_sender_config, int> port;
  jb::config_attribute<udp_sender_config, std::string> address;
  jb::config_attribute<udp_sender_config, bool> enable_loopback;
  jb::config_attribute<udp_sender_config, int> hops;
  jb::config_attribute<udp_sender_config, std::string> outbound_interface;
  jb::config_attribute<udp_sender_config, bool> broadcast;
};

} // namespace itch5
} // namespace jb

#endif // jb_itch5_udp_sender_config_hpp
