#ifndef jb_convert_cpu_set_hpp
#define jb_convert_cpu_set_hpp

#include <jb/cpu_set.hpp>
#include <yaml-cpp/yaml.h>

namespace YAML {

/**
 * Specialize YAML conversions for jb::cpu_set.
 *
 * Arguably this might belong in the header defining jb::cpu_set,
 * but this is specific to using a cpu_set in configuration objects.
 */
template <>
struct convert<jb::cpu_set> {
  static bool decode(YAML::Node node, jb::cpu_set& rhs) {
    rhs = jb::cpu_set::parse(node.as<std::string>());
    return true;
  }

  static YAML::Node encode(jb::cpu_set const& x) {
    return YAML::Node(x.as_list_format());
  }
};

} // namespace YAML

#endif // jb_convert_cpu_set
