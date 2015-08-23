/**
 * @file
 *
 * This is an example of how to create and use configuration objects in JayBeams.
 */
#include <jb/config_object.hpp>
#include <jb/strtonum.hpp>

#include <iostream>

// ... creating configuration components takes very little boiler
// plate code... first your class must derive from config_object:
/// A configuration class for threads
class thread_config : public jb::config_object {
 public:
  thread_config()
      // ... you must describe how the attributes are mapped to
      // names ...
      : name(desc("name"), this, "")
        // ... if you wish, you can define default values for your
        // attributes ...
      , scheduler(desc("scheduler"), this, "OTHER")
      , priority(desc("priority"), this, "MIN")
      , affinity(desc("affinity"), this, ~0) {
  }
  // ... this makes your class behave like a canonical object, i.e.,
  // it has copy constructor, move constructor, and the corresponding
  // assighment operators ...
  config_object_constructors(thread_config);

  // ... all your attributes are defined as elements next ...
  jb::config_attribute<thread_config,std::string> name;
  jb::config_attribute<thread_config,std::string> scheduler;
  jb::config_attribute<thread_config,std::string> priority;
  // ... you can use different types ...
  jb::config_attribute<thread_config,int> affinity;

  // ... you can add convenience functions to make the configuration
  // class easier to use ...
  int native_scheduling_policy() const;
  int native_priority() const;
};

// ... config objects can be composed ...
/// A configuration class for a worker with two threads.
class worker_config : public jb::config_object {
 public:
  worker_config()
      // ... the description of the attributes can include a help
      // message ...
      : cl_device_name(desc("cl-device-name").help(
          "The name of the CL device used by this worker"), this)
        // ... your description of the attributes can include a
        // "class", allowing you to change all the attributes with
        // that class in a single configuration section (more on this
        // later) ...
      , reader(desc("reader", "thread_config"), this)
      , writer(desc("writer", "thread_config"), this) {
  }
  config_object_constructors(worker_config);

  jb::config_attribute<worker_config,std::string> cl_device_name;
  // ... just create an attribute that holds a config object ...
  jb::config_attribute<worker_config,thread_config> reader;
  jb::config_attribute<worker_config,thread_config> writer;
};

// ... the composition can be arbitrarily deeep ...
/// The main configuration class for this example program.
class program_config : public jb::config_object {
 public:
  program_config()
      : securities(desc("securities").help(
          "The list of securities to process"), this)
      , workers(desc("workers", "worker_config"), this) {
  }

  // ... notice how we can have lists (or vectors) of attributes ...
  jb::config_attribute<program_config,std::vector<std::string>> securities;
  jb::config_attribute<program_config,std::vector<worker_config>> workers;
};

int main(int argc, char* argv[]) try {
  // ... to use your configuration just create one, it is initialized
  // to whatever defaults you defined ...
  program_config config;

  // ... you can change parts of the configuration programatically ...
  std::vector<worker_config> v;
  v.push_back(std::move(worker_config().cl_device_name("Tahiti")));
  config.workers(std::move(v));

  // ... and/or load the rest from a configuration file ...
  config.load_overrides(argc, argv, "my_program.yaml", "MY_PROGRAM_ROOT");

  // ... and then access them as usual ...
  std::cout << "securities = [";
  char const* sep = "";
  for (auto s : config.securities()) {
    std::cout << sep << s;
    sep = ", ";
  }
  std::cout << "]\n";
  int cnt = 0;
  for (auto w : config.workers()) {
    std::cout << "worker." << cnt << ".cl-device-name = "
              << w.cl_device_name() << std::endl;
    cnt++;
  }

  return 0;
} catch(jb::usage const& ex) {
  std::cerr << ex.what() << std::endl;
  return ex.exit_status();
} catch(std::exception const& ex) {
  std::cerr << "standard exception raised: " << ex.what() << std::endl;
  return 1;
} catch(...) {
  std::cerr << "unknown exception raised" << std::endl;
  return 1;
}

int thread_config::native_scheduling_policy() const {
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

int thread_config::native_priority() const {
  int policy = native_scheduling_policy();
  if (priority() == "MIN") {
    return sched_get_priority_min(policy);
  }
  if (priority() == "MAX") {
    return sched_get_priority_max(policy);
  }
  if (priority() == "MID") {
    return (sched_get_priority_max(policy)
            + sched_get_priority_min(policy)) / 2;
  }
  int prio;
  if (jb::strtonum(priority(), prio)) {
    return prio;
  }
  std::string msg("Invalid scheduling priority: ");
  msg += priority();
  throw std::runtime_error(msg);
}
