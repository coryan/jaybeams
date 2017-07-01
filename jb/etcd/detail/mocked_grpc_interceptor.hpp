#ifndef jb_etcd_detail_mocked_grpc_interceptor_hpp
#define jb_etcd_detail_mocked_grpc_interceptor_hpp

#include <jb/etcd/detail/async_ops.hpp>

#include <gmock/gmock.h>
#include <grpc++/grpc++.h>
#include <memory>

namespace jb {
namespace etcd {
namespace detail {

/**
 *
 */
struct mocked_grpc_interceptor {
  mocked_grpc_interceptor()
      : shared_mock(new mocked) {
  }

  /// Intercept posting of asynchronous RPC operations
  template <typename C, typename M, typename op_type>
  void async_rpc(
      C* async_client, M C::*call, std::shared_ptr<op_type>& op,
      grpc::CompletionQueue* cq, void* tag) {
    shared_mock->async_rpc(op);
  }

  /// Intercept creation of asynchronous rdwr RPC streams.
  template <typename C, typename M, typename op_type>
  void async_create_rdwr_stream(
      C* async_client, M C::*call, std::shared_ptr<op_type>& op,
      grpc::CompletionQueue* cq, void* tag) {
    shared_mock->async_create_rdwr_stream(op);
  }

  struct mocked {
    MOCK_CONST_METHOD1(async_rpc, void(std::shared_ptr<base_async_op> op));
    MOCK_CONST_METHOD1(
        async_create_rdwr_stream, void(std::shared_ptr<base_async_op> op));
  };

  std::shared_ptr<mocked> shared_mock;
};

} // namespace detail
} // namespace etcd
} // namespace jb

#endif // jb_etcd_detail_mocked_grpc_interceptor_hpp
