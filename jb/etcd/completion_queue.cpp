#include "jb/etcd/completion_queue.hpp"

namespace jb {
namespace etcd {

completion_queue::completion_queue()
    : queue_() {
}

void completion_queue::run() {
  void* tag = nullptr;
  bool ok = false;
  while (queue_.Next(&tag, &ok)) {
    if (not ok or tag == nullptr) {
      // TODO() - I think tag == nullptr should never happen,
      // consider using JB_ASSERT()
      continue;
    }
    // ... tag must be a pointer to std::function<void()> see the
    // comments for tag_set_timer_ and friends to understand why ...
    auto callback = static_cast<std::function<void()>*>(tag);
    if (callback and *callback) {
      (*callback)();
    }
  }
}

void completion_queue::shutdown() {
  queue_.Shutdown();
}

} // namespace etcd
} // namespace jb
