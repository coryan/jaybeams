/**
 * @file
 *
 * Helper functions to log errors when setting std::promise.
 */
#ifndef jb_etcd_log_promise_errors_hpp
#define jb_etcd_log_promise_errors_hpp

#include <future>

namespace jb {
namespace etcd {

/// Implementation for log_promise_errors()
void log_promise_errors_impl(
    std::exception_ptr eptr, std::exception_ptr promise_exception,
    std::string header, std::string where);

std::string log_promise_errors_text(
    std::exception_ptr eptr, std::exception_ptr promise_exception,
    std::string header, std::string where);

/// Set a std::promise to an exception status and log errors if we
/// cannot ...
template <typename T>
void log_promise_errors(
    std::promise<T>& p, std::exception_ptr eptr, std::string header,
    std::string where) try {
  p.set_exception(eptr);
} catch (...) {
  log_promise_errors_impl(
      eptr, std::current_exception(), std::move(header), std::move(where));
}

} // namespace etcd
} // namespace jb

#endif // jb_etcd_log_promise_errors_hpp
