#ifndef jb_opencl_config_hpp
#define jb_opencl_config_hpp

#include <jb/config_object.hpp>

namespace jb {
namespace opencl {

/**
 * Configure the OpenCL device / context options.
 */
class config : public jb::config_object {
public:
  config();
  config_object_constructors(config);

  jb::config_attribute<config, std::string> device_name;
};

} // namespace opencl
} // namespace jb

#endif // jb_opencl_config_hpp
