#ifndef jb_etcd_grpc_errors_hpp
#define jb_etcd_grpc_errors_hpp
/**
 * @file Helper functions to handle errors reported by gRPC++
 */

#include <google/protobuf/message.h>
#include <iostream>

namespace jb {
namespace etcd {

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
