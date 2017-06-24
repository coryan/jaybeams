#ifndef jb_etcd_active_completion_queue_hpp
#define jb_etcd_active_completion_queue_hpp

#include <jb/etcd/completion_queue.hpp>
#include <thread>

namespace jb {
namespace etcd {

/**
 * Create a command_queue with an associated thread running its loop.
 *
 * This deals with the awful order of construction problems.  It holds
 * a completion queue and a thread running the event loop for the
 * queue.  On destruction it shutdown the completion queue first, and
 * then joins the thread.
 */
class active_completion_queue {
public:
  /// Constructor, creates new completion queue and thread.
  active_completion_queue()
      : queue_(std::make_shared<completion_queue>())
      , thread_([q = queue()]() { q->run(); })
      , join_(&thread_)
      , shutdown_(queue()) {
  }

  /// Constructor from existing queue and thread.  Assumes thread
  /// calls q->run().
  active_completion_queue(std::shared_ptr<completion_queue> q, std::thread&& t)
      : queue_(std::move(q))
      , thread_(std::move(t))
      , join_(&thread_)
      , shutdown_(queue()) {
  }

  active_completion_queue(active_completion_queue&& rhs)
      : queue_()
      , thread_()
      , join_(&thread_)
      , shutdown_(queue()) {
    *this = std::move(rhs);
  }
  active_completion_queue& operator=(active_completion_queue&& rhs) {
    queue_ = std::move(rhs.queue_);
    thread_ = std::move(rhs.thread_);
    shutdown_.queue = queue_;
    return *this;
  }
  active_completion_queue(active_completion_queue const&) = delete;
  active_completion_queue& operator=(active_completion_queue const&) = delete;

  operator grpc::CompletionQueue*() {
    return static_cast<grpc::CompletionQueue*>(*queue_);
  }

private:
  /// A helper for the lambdas in the constructor
  std::shared_ptr<completion_queue> queue() {
    return queue_;
  }

  /// Shutdown a completion queue
  struct defer_shutdown {
    defer_shutdown(std::shared_ptr<completion_queue> q)
        : queue(std::move(q)) {
    }
    ~defer_shutdown();
    std::shared_ptr<completion_queue> queue;
  };

  /// Join a thread
  struct defer_join {
    defer_join(std::thread* t)
        : thread(t) {
    }
    ~defer_join();
    std::thread* thread;
  };

private:
  std::shared_ptr<completion_queue> queue_;
  std::thread thread_;
  defer_join join_;
  defer_shutdown shutdown_;
};

} // namespace etcd
} // namespace jb

#endif // jb_etcd_active_completion_queue_hpp
