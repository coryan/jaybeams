#include "jb/etcd/active_completion_queue.hpp"

namespace jb {
namespace etcd {

active_completion_queue::defer_shutdown::~defer_shutdown() {
  queue->shutdown();
}

active_completion_queue::defer_join::~defer_join() {
  if (thread->joinable()) {
    thread->join();
  }
}

} // namespace etcd
} // namespace jb
