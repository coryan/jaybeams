/**
 * @file
 *
 * Helper functions to handle errors reported by gRPC++
 */
#ifndef jb_etcd_grpc_errors_hpp
#define jb_etcd_grpc_errors_hpp

#include <jb/etcd/detail/grpc_errors_annotations.hpp>

#include <google/protobuf/message.h>
#include <grpc++/grpc++.h>
#include <sstream>

namespace jb {
namespace etcd {

/**
 * Format a gRPC error into a std::ostream.
 *
 * This is often converted into an exception, but it is easier to
 * unit tests if separated.
 *
 * @param where a string to let the user know where the error took
 * place.
 * @param status the status to format
 */
template <typename Location, typename... Annotations>
void check_grpc_status(
    grpc::Status const& status, Location const& where, Annotations&&... a) {
  if (status.ok()) {
    return;
  }
  std::ostringstream os;
  os << where << " grpc error: " << status.error_message() << " ["
     << status.error_code() << "]";
  detail::append_annotations(os, std::forward<Annotations>(a)...);
  throw std::runtime_error(os.str());
}

/**
 * Print a protobuf on a std::ostream.
 *
 * Uses google::protobuf::TextFormat::PrintToString to print a
 * protobuf.  Typically one would use is as in:
 *
 * @code
 * blah::ProtoName const& proto = ...;
 * std::ostream& os = ...;
 *
 * os << "foo " << 1 << print_to_stream(proto) << " blah";
 * @endcode
 */
struct print_to_stream {
  explicit print_to_stream(google::protobuf::Message const& m)
      : msg(m) {
  }

  google::protobuf::Message const& msg;
};

/// Streaming operator
std::ostream& operator<<(std::ostream& os, print_to_stream const& x);

} // namespace etcd
} // namespace jb

#endif // jb_etcd_grpc_errors_hpp
