#ifndef jb_launch_thread_hpp
#define jb_launch_thread_hpp

#include <jb/detail/thread_setup_wrapper.hpp>
#include <thread>
#include <type_traits>

namespace jb {

/**
 * Create a new thread, configure it as desired, and then call a
 * user-defined function.
 */
template <typename Function, typename... A>
void launch_thread(
    std::thread& t, thread_config const& config, Function&& f, A&&... a) {
  auto c = detail::make_thread_setup_wrapper(
      config, std::bind(std::forward<Function>(f), std::forward<A>(a)...));

  std::thread tmp(c);
  t = std::move(tmp);
};

} // namespace jb

#endif // jb_launch_thread_hpp
