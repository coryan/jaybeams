#include "jb/thread_config.hpp"
#include <jb/strtonum.hpp>

#include <sched.h>
#include <stdexcept>

namespace jb {
namespace defaults {

#ifndef JB_DEFAULT_thread_config_scheduler
#define JB_DEFAULT_thread_config_scheduler "OTHER"
#endif // JB_DEFAULT_thread_config_scheduler

#ifndef JB_DEFAULT_thread_config_priority
#define JB_DEFAULT_thread_config_priority "MIN"
#endif // JB_DEFAULT_thread_config_priority

#ifndef JB_DEFAULT_ignore_setup_errors
#define JB_DEFAULT_ignore_setup_errors true
#endif // JB_DEFAULT_ignore_setup_errors

std::string thread_config_scheduler = JB_DEFAULT_thread_config_scheduler;
std::string thread_config_priority = JB_DEFAULT_thread_config_priority;
bool ignore_setup_errors = JB_DEFAULT_ignore_setup_errors;

} // namespace defaults
} // namespace jb

jb::thread_config::thread_config()
    : name(desc("name").help("The name of this thread"), this)
    , scheduler(
          desc("scheduler").help("The scheduling policy for this thread"), this,
          defaults::thread_config_scheduler)
    , priority(
          desc("priority")
              .help(
                  "The priority for this thread. "
                  "Use MIN/MID/MAX for the minimum, midpoint and maximum "
                  "priorities "
                  "in the scheduling class. "
                  "Use a number to set the specific priority"),
          this, defaults::thread_config_priority)
    , affinity(
          desc("affinity", "cpu_set")
              .help(
                  "The CPU affinity for this thread. "
                  "If none is set, the thread keeps its default affinity "
                  "settings."),
          this)
    , ignore_setup_errors(
          desc("ignore-setup-errors")
              .help("Ignore errors changing thread parameters."),
          this, defaults::ignore_setup_errors) {
}

int jb::thread_config::native_scheduling_policy() const {
  if (scheduler() == "RR") {
    return SCHED_RR;
  }
  if (scheduler() == "FIFO") {
    return SCHED_FIFO;
  }
  if (scheduler() == "OTHER") {
    return SCHED_OTHER;
  }
  if (scheduler() == "BATCH") {
    return SCHED_BATCH;
  }
  if (scheduler() == "IDLE") {
    return SCHED_IDLE;
  }
  std::string msg("Unknown scheduling policy: ");
  msg += scheduler();
  throw std::runtime_error(msg);
}

int jb::thread_config::native_priority() const {
  int policy = native_scheduling_policy();
  if (priority() == "MIN") {
    return sched_get_priority_min(policy);
  }
  if (priority() == "MAX") {
    return sched_get_priority_max(policy);
  }
  if (priority() == "MID") {
    return (sched_get_priority_max(policy) + sched_get_priority_min(policy)) /
           2;
  }
  int prio;
  if (jb::strtonum(priority(), prio)) {
    return prio;
  }
  std::string msg("Invalid scheduling priority: ");
  msg += priority();
  throw std::runtime_error(msg);
}
