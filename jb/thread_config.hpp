#ifndef jb_thread_config_hpp
#define jb_thread_config_hpp

#include <jb/config_object.hpp>
#include <jb/convert_cpu_set.hpp>

namespace jb {

/**
 * Hold the configuration to initialize threads.
 *
 * After a thread is started we want to be able to consistently
 * configure its scheduling policy, priority, processor affinity,
 * etc.  This class is used to contain such configurations.
 */
class thread_config : public jb::config_object {
public:
  /// Constructor.
  thread_config();
  config_object_constructors(thread_config);

  jb::config_attribute<thread_config, std::string> name;
  jb::config_attribute<thread_config, std::string> scheduler;
  jb::config_attribute<thread_config, std::string> priority;
  jb::config_attribute<thread_config, jb::cpu_set> affinity;
  jb::config_attribute<thread_config, bool> ignore_setup_errors;

  // ... you can add convenience functions to make the configuration
  // class easier to use ...
  int native_scheduling_policy() const;
  int native_priority() const;
};

} // namespace jb

#endif //  jb_thread_config_hpp
