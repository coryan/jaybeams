#ifndef jb_testing_microbenchmark_config_hpp
#define jb_testing_microbenchmark_config_hpp

#include <jb/config_object.hpp>
#include <string>

namespace jb {
namespace testing {

/**
 * Configure a micro-benchmark.
 */
class microbenchmark_config : public jb::config_object {
 public:
  /**
   * Default constructor.
   *
   * Initialize all values to the default settings.
   */
  microbenchmark_config();
  config_object_constructors(microbenchmark_config);

  jb::config_attribute<microbenchmark_config,int> warmup_iterations;
  jb::config_attribute<microbenchmark_config,int> iterations;
  jb::config_attribute<microbenchmark_config,int> size;
  jb::config_attribute<microbenchmark_config,bool> verbose;
  jb::config_attribute<microbenchmark_config,std::string> test_case;
};

} // namespace testing
} // namespace jb

#endif // jb_testing_microbenchmark_config_hpp
