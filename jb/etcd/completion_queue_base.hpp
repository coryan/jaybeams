#ifndef jb_etcd_completion_queue_base_hpp
#define jb_etcd_completion_queue_base_hpp

#include <jb/etcd/completion_queue_base.hpp>
#include <jb/etcd/detail/async_ops.hpp>
#include <jb/etcd/detail/default_grpc_interceptor.hpp>

#include <atomic>
#include <chrono>
#include <functional>
#include <memory>
#include <mutex>
#include <unordered_map>

namespace jb {
namespace etcd {

/**
 * The base class for the grpc::CompletionQueue wrappers.
 *
 * Refactor code common to all jb::etcd::completion_queue<> template
 * instantiations.
 */
class completion_queue_base {
public:
  /// Stop the loop periodically to check if we should shutdown.
  // TODO() - the timeout should be configurable ...
  static std::chrono::milliseconds constexpr loop_timeout{250};

  completion_queue_base();
  virtual ~completion_queue_base();

  /// The underlying completion queue
  grpc::CompletionQueue& raw() {
    return queue_;
  }

  /// Run the completion queue loop.
  void run();

  /// Shutdown the completion queue loop.
  void shutdown();

  operator grpc::CompletionQueue*() {
    return &queue_;
  }

protected:
  /// The underlying completion queue pointer for the gRPC APIs.
  grpc::CompletionQueue* cq() {
    return &queue_;
  }

  /// Create an operation and do the common initialization
  template <typename op_type, typename Functor>
  std::shared_ptr<op_type> create_op(std::string name, Functor&& f) const {
    auto op = std::make_shared<op_type>();
    op->callback = [functor = std::move(f)](
        detail::base_async_op & bop, bool ok) {
      auto const& op = dynamic_cast<op_type const&>(bop);
      functor(op, ok);
    };
    op->name = std::move(name);
    return op;
  }

  /// Save a newly created operation and return its gRPC tag.
  void*
  register_op(char const* where, std::shared_ptr<detail::base_async_op> op);

  /// Get an operation given its gRPC tag.
  std::shared_ptr<detail::base_async_op> unregister_op(void* tag);

private:
  mutable std::mutex mu_;
  using pending_ops_type =
      std::unordered_map<std::intptr_t, std::shared_ptr<detail::base_async_op>>;
  pending_ops_type pending_ops_;

  grpc::CompletionQueue queue_;
  std::atomic<bool> shutdown_;
};

} // namespace etcd
} // namespace jb

#endif // jb_etcd_completion_queue_base_hpp
