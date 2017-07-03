#ifndef jb_opencl_microbenchmark_config_hpp
#define jb_opencl_microbenchmark_config_hpp

#include <jb/opencl/config.hpp>
#include <jb/testing/microbenchmark_config.hpp>
#include <jb/log.hpp>

namespace jb {
namespace opencl {
/**
 * The configuration shared by all OpenCL microbenchmarks.
 */
class microbenchmark_config : public jb::config_object {
public:
  microbenchmark_config();
  config_object_constructors(microbenchmark_config);

  jb::config_attribute<
      microbenchmark_config, jb::testing::microbenchmark_config>
      microbenchmark;
  jb::config_attribute<microbenchmark_config, jb::log::config> log;
  jb::config_attribute<microbenchmark_config, jb::opencl::config> opencl;
};

} // namespace opencl
} // namespace jb

#endif // jb_opencl_microbenchmark_config_hpp
