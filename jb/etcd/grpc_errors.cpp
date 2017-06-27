#include "jb/etcd/grpc_errors.hpp"

#include <google/protobuf/text_format.h>
#include <string>

namespace jb {
namespace etcd {

std::ostream& operator<<(std::ostream& os, print_to_stream const& x) {
  std::string formatted;
  if (not google::protobuf::TextFormat::PrintToString(x.msg, &formatted)) {
    return os << "[error-formatting-protobuf]";
  }
  return os << formatted;
}

} // namespace etcd
} // namespace jb
