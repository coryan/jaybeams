#include "jb/etcd/active_completion_queue.hpp"
#include <jb/log.hpp>

namespace jb {
namespace etcd {
active_completion_queue::~active_completion_queue() {
  JB_LOG(info) << "delete active completion queue";
}

active_completion_queue::defer_shutdown::~defer_shutdown() {
  if (queue) {
    JB_LOG(info) << "shutdown active completion queue";
    queue->shutdown();
  }
}

active_completion_queue::defer_join::~defer_join() {
  if (thread->joinable()) {
    JB_LOG(info) << "join active completion queue";
    thread->join();
  }
}

} // namespace etcd
} // namespace jb
