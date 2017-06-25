#include "jb/etcd/completion_queue.hpp"
#include <jb/log.hpp>

namespace jb {
namespace etcd {

std::chrono::milliseconds constexpr completion_queue::loop_timeout;

completion_queue::completion_queue()
    : mu_()
    , pending_ops_()
    , queue_()
    , shutdown_(false) {
}

completion_queue::~completion_queue() {
  if (not pending_ops_.empty()) {
    // At this point there is not much to do, we could try to call the
    // operations and tell them they are cancelled, but that is risky,
    // they might be pointing to objects already deleted.  We print
    // the best debug message we can, and just continue on our way to
    // a likely crash.
    // TODO() - consider calling std::terminate() or raising an
    // exception (which requires declaring this destructor
    // noexcept(false).
    std::ostringstream os;
    for (auto const& op : pending_ops_) {
      os << op.second->name << "\n";
    }
    JB_LOG(error) << " completion queue deleted while holding "
                  << pending_ops_.size() << " pending operations: " << os.str();
  }
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

    // ... try to find the operation in our list of known operations ...
    std::shared_ptr<detail::base_async_op> op;
    {
      std::lock_guard<std::mutex> lock(mu_);
      pending_ops_type::iterator i =
          pending_ops_.find(reinterpret_cast<std::intptr_t>(tag));
      if (i != pending_ops_.end()) {
        op = i->second;
        pending_ops_.erase(i);
      }
    }
    if (op) {
      // ... it was there, now it is removed, and the lock is safely
      // unreleased, call it ...
      op->callback(*op, ok);
    } else {
      // TODO() - this is here while I refactor the code to move all
      // async_op creation/destruction to this class (from free
      // functions)...
      auto callback = static_cast<std::function<void(bool)>*>(tag);
      (*callback)(ok);
    }
  }
}

void completion_queue::shutdown() {
  JB_LOG(trace) << "shutting down queue";
  shutdown_.store(true);
  queue_.Shutdown();
}

void* completion_queue::register_op(
    char const* where, std::shared_ptr<detail::base_async_op> op) {
  void* tag = static_cast<void*>(op.get());
  auto key = reinterpret_cast<std::intptr_t>(tag);
  std::lock_guard<std::mutex> lock(mu_);
  auto r = pending_ops_.emplace(key, op);
  if (r.second == false) {
    std::ostringstream os;
    os << where << " duplicate operation (" << std::hex << key << ") for "
       << op->name;
    throw std::runtime_error(os.str());
  }
  return tag;
}

} // namespace etcd
} // namespace jb
