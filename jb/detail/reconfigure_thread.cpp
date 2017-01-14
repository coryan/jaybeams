#include "jb/detail/reconfigure_thread.hpp"
#include <jb/detail/os_error.hpp>

#include <pthread.h>
#include <sched.h>

void jb::detail::reconfigure_this_thread(jb::thread_config const& config) {
  pthread_t self = pthread_self();

  sched_param param;
  param.sched_priority = config.native_priority();
  int r =
      pthread_setschedparam(self, config.native_scheduling_policy(), &param);
  if (not config.ignore_setup_errors()) {
    os_check_error(r, "reconfigure_this_thread() - setting scheduling");
  }

  if (config.affinity().count() > 0) {
    r = pthread_setaffinity_np(
        self, sizeof(cpu_set_t),
        const_cast<cpu_set_t*>(config.affinity().native_handle()));
    if (not config.ignore_setup_errors()) {
      os_check_error(r, "reconfigure_this_thread() - setting affinity");
    }
  }

  if (config.name() != "") {
    r = pthread_setname_np(self, config.name().c_str());
    if (not config.ignore_setup_errors()) {
      os_check_error(r, "reconfigure_this_thread() - setting name");
    }
  }
}
