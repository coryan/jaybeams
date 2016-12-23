#ifndef jb_detail_reconfigure_thread_hpp
#define jb_detail_reconfigure_thread_hpp

#include <jb/thread_config.hpp>

namespace jb {
namespace detail {

/**
 * Change the current thread parameters based on the configuration.
 *
 * @param config the thread configuration
 * @throw std::exception if the thread cannot be configured as desired
 */
void reconfigure_this_thread(thread_config const& config);

} // namespace jb
} // namespace jb

#endif // jb_detail_reconfigure_thread_hpp
