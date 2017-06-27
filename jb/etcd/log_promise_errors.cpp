#include "jb/etcd/log_promise_errors.hpp"
#include <jb/log.hpp>

namespace jb {
namespace etcd {

std::string log_promise_errors_text(
    std::exception_ptr eptr, std::exception_ptr promise_eptr,
    std::string header, std::string where) {
  auto exception_text = [](std::exception_ptr eptr) {
    if (not eptr) {
      return std::string("no exception");
    }
    try {
      std::rethrow_exception(eptr);
    } catch (std::exception const& ex) {
      return std::string("std::exception<") + ex.what() + ">";
    } catch (...) {
      return std::string("unknown exception");
    }
    /*NOT REACHED*/ return std::string("no exception");
  };

  std::string exception_description = exception_text(promise_eptr);
  std::string eptr_description = exception_text(eptr);

  std::ostringstream os;
  os << header << ": " << exception_description << " raised by promise in "
     << where << " while setting the promise to exception=" << eptr_description;
  return os.str();
}

void log_promise_errors_impl(
    std::exception_ptr eptr, std::exception_ptr promise_eptr,
    std::string header, std::string where) {
  JB_LOG(info) << log_promise_errors_text(eptr, promise_eptr, header, where);
}

} // namespace etcd
} // namespace jb
