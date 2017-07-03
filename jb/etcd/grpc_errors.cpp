#include "jb/etcd/grpc_errors.hpp"

#include <google/protobuf/text_format.h>
#include <string>

namespace jb {
namespace etcd {

std::ostream& operator<<(std::ostream& os, print_to_stream const& x) {
  // Print and ignore errors, on failure we just get an empty string ...
  std::string formatted;
  (void)google::protobuf::TextFormat::PrintToString(x.msg, &formatted);
  return os << formatted;
}

} // namespace etcd
} // namespace jb
