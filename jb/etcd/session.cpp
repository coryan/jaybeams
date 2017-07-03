#include "jb/etcd/session.hpp"
#include <jb/assert_throw.hpp>

#include <iostream>

namespace jb {
namespace etcd {

int constexpr session::keep_alives_per_ttl;

session::session(
    std::unique_ptr<etcdserverpb::Lease::Stub> lease_stub,
    std::chrono::milliseconds desired_TTL, std::uint64_t lease_id)
    : mu_()
    , state_(state::constructing)
    , lease_client_(std::move(lease_stub))
    , ka_stream_()
    , lease_id_(lease_id)
    , desired_TTL_(desired_TTL)
    , actual_TTL_(desired_TTL)
    , current_timer_() {
}

session::~session() noexcept(false) {
}

std::ostream& operator<<(std::ostream& os, session::state s) {
  static char const* names[] = {
      "constructing", "connecting", "connected", "shuttingdown", "shutdown",
  };
  JB_ASSERT_THROW(
      0 <= int(s) and std::size_t(s) < sizeof(names) / sizeof(names[0]));
  return os << names[int(s)];
}

} // namespace etcd
} // namespace jb
