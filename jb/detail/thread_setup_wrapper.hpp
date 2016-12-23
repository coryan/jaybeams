#ifndef jb_detail_thread_setup_wrapper_hpp
#define jb_detail_thread_setup_wrapper_hpp

#include <jb/detail/reconfigure_thread.hpp>
#include <jb/log.hpp>
#include <functional>
#include <thread>

namespace jb {
namespace detail {

/**
 * Hold data to startup a thread in JayBeams.
 *
 * We want to launch all threads using a common configuration
 * function.  To match the semantics of std::thread, we need to copy
 * the thread functor and its parameters exactly once.  This class
 * wraps the relevant bits in a shared_ptr<> to avoid excessive
 * copying.
 */
template <typename Callable>
struct thread_setup_wrapper {
public:
  thread_setup_wrapper(jb::thread_config const& config, Callable&& c)
      : config_(config)
      , callable_(std::make_shared<Callable>(std::forward<Callable>(c))) {
  }
  virtual void operator()() {
    try {
      reconfigure_this_thread(config_);
      (*callable_)();
    } catch (std::exception const& ex) {
      JB_LOG(warning) << "standard exception raised: " << ex.what()
                      << " in thread reconfiguration";
    } catch (...) {
      JB_LOG(warning) << "unknown exception raised"
                      << " in thread reconfiguration";
    }
  }

private:
  jb::thread_config config_;
  std::shared_ptr<Callable> callable_;
};

/**
 * Create the right type of thread_setup_wrapper<>.
 */
template <typename Callable>
thread_setup_wrapper<Callable>
make_thread_setup_wrapper(thread_config const& config, Callable&& c) {
  return thread_setup_wrapper<Callable>(config, std::forward<Callable>(c));
}

} // namespace detail
} // namespace jb

#endif // jb_detail_thread_setup_wrapper_hpp
