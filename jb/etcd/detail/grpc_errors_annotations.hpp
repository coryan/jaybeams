#ifndef jb_etcd_detail_grpc_errors_annotations_hpp
#define jb_etcd_detail_grpc_errors_annotations_hpp

#include <iostream>
#include <utility>

namespace jb {
namespace etcd {
namespace detail {

/// Helper function to jb::etcd::check_grpc_status, no annotations case.
inline void append_annotations(std::ostream& os) {
}

/// Helper function to jb::etcd::check_grpc_status, single annotations case.
template <typename T>
inline void append_annotations(std::ostream& os, T&& t) {
  os << t;
}

/// Helper function to jb::etcd::check_grpc_status, annotations list case.
template <typename H, typename... Tail>
inline void append_annotations(std::ostream& os, H&& h, Tail&&... t) {
  append_annotations(os << h, std::forward<Tail>(t)...);
}

} // namespace detail
} // namespace etcd
} // namespace jb

#endif // jb_etcd_detail_grpc_errors_annotations_hpp
