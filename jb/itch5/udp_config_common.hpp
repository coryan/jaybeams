#ifndef jb_itch5_udp_config_common_hpp
#define jb_itch5_udp_config_common_hpp

#include <jb/config_object.hpp>

namespace jb {
namespace itch5 {

/**
 * Common configuration parameters for both UDP senders and receivers.
 */
class udp_config_common : public jb::config_object {
public:
  udp_config_common();
  config_object_constructors(udp_config_common);

  jb::config_attribute<udp_config_common, bool> debug;
  jb::config_attribute<udp_config_common, bool> do_not_route;
  jb::config_attribute<udp_config_common, bool> linger;
  jb::config_attribute<udp_config_common, int> linger_seconds;
  jb::config_attribute<udp_config_common, bool> reuse_address;
  jb::config_attribute<udp_config_common, int> receive_buffer_size;
  jb::config_attribute<udp_config_common, int> receive_low_watermark;
  jb::config_attribute<udp_config_common, int> send_buffer_size;
  jb::config_attribute<udp_config_common, int> send_low_watermark;
};

} // namespace itch5
} // namespace jb

#endif // jb_itch5_udp_config_common_hpp
