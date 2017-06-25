#include "jb/etcd/completion_queue.hpp"
#include <jb/log.hpp>

namespace jb {
namespace etcd {

std::chrono::milliseconds constexpr completion_queue::loop_timeout;

completion_queue::completion_queue()
    : queue_()
    , shutdown_(false) {
}

completion_queue::~completion_queue() {
}

void completion_queue::run() {
  void* tag = nullptr;
  bool ok = false;
  while (not shutdown_.load()) {
    auto deadline = std::chrono::system_clock::now() + loop_timeout;

    auto status = queue_.AsyncNext(&tag, &ok, deadline);
    if (status == grpc::CompletionQueue::SHUTDOWN) {
      JB_LOG(trace) << "shutdown, exit loop";
      break;
    }
    if (status == grpc::CompletionQueue::TIMEOUT) {
      JB_LOG(trace) << "timeout, continue loop";
      continue;
    }
    if (tag == nullptr) {
      // TODO() - I think tag == nullptr should never happen,
      // consider using JB_ASSERT()
      continue;
    }
    // ... tag must be a pointer to std::function<void()> see the
    // comments for tag_set_timer_ and friends to understand why ...
    auto callback = static_cast<std::function<void(bool)>*>(tag);
    (*callback)(ok);
  }
}

void completion_queue::shutdown() {
  JB_LOG(trace) << "shutting down queue";
  shutdown_.store(true);
  queue_.Shutdown();
}

} // namespace etcd
} // namespace jb
