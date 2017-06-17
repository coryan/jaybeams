#ifndef jb_itch5_udp_receiver_config_hpp
#define jb_itch5_udp_receiver_config_hpp

#include <jb/config_object.hpp>

namespace jb {
namespace itch5 {

/**
 * A configuration object for UDP receivers.
 */
class udp_receiver_config : public jb::config_object {
public:
  udp_receiver_config();
  config_object_constructors(udp_receiver_config);

  jb::config_attribute<udp_receiver_config, int> port;
  jb::config_attribute<udp_receiver_config, std::string> listen_address;
  jb::config_attribute<udp_receiver_config, std::string> receive_address;
};

} // namespace itch5
} // namespace jb

#endif // jb_itch5_udp_receiver_config_hpp
