#ifndef jb_convert_severity_level_hpp
#define jb_convert_severity_level_hpp

#include <jb/severity_level.hpp>
#include <yaml-cpp/yaml.h>

namespace YAML {
/**
 * Specialize YAML conversions for jb::severity_level.
 *
 * Arguably this might belong in the header defining jb::severity_level,
 * but this is specific to using a jb::severity_level in configuration objects.
 */
template<>
struct convert<jb::severity_level> {
  static bool decode(YAML::Node node, jb::severity_level& rhs) {
    jb::parse(rhs, node.as<std::string>());
    return true;
  }

  static YAML::Node encode(jb::severity_level const& x) {
    return YAML::Node(jb::get_name(x));
  }
};
} // namespace YAML

#endif // jb_convert_severity_level_hpp
