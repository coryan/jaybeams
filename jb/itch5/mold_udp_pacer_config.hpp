#ifndef jb_itch5_mold_udp_pacer_config_hpp
#define jb_itch5_mold_udp_pacer_config_hpp

#include <jb/config_object.hpp>

namespace jb {
namespace itch5 {

/**
 * Configuration object for the jb::itch5::mold_udp_pacer class.
 */
class mold_udp_pacer_config : public jb::config_object {
public:
  mold_udp_pacer_config();
  config_object_constructors(mold_udp_pacer_config);

  void validate() const override;

  jb::config_attribute<mold_udp_pacer_config, int> maximum_delay_microseconds;
  jb::config_attribute<mold_udp_pacer_config, int> maximum_transmission_unit;
};

} // namespace itch5
} // namespace jb

#endif // jb_itch5_mold_udp_pacer_config_hpp
