#ifndef jb_etcd_detail_default_grpc_interceptor_hpp
#define jb_etcd_detail_default_grpc_interceptor_hpp

#include <grpc++/grpc++.h>
#include <memory>

namespace jb {
namespace etcd {
namespace detail {

/**
 *
 */
struct default_grpc_interceptor {
  /// Post a asynchronous RPC operation via the completion queue
  template <typename C, typename M, typename op_type>
  void async_rpc(
      C* async_client, M C::*call, std::shared_ptr<op_type>& op,
      grpc::CompletionQueue* cq, void* tag) {
    op->rpc = (async_client->*call)(&op->context, op->request, cq);
    op->rpc->Finish(&op->response, &op->status, tag);
  }
};

} // namespace detail
} // namespace etcd
} // namespace jb

#endif // jb_etcd_detail_default_grpc_interceptor_hpp
