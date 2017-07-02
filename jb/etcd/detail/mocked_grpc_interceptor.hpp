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

  /// Intercept an asynchronous Write() operation over a rdwr RPC stream
  template <typename W, typename R, typename op_type>
  void async_write(
      async_rdwr_stream<W, R> const& stream, std::shared_ptr<op_type>& op,
      void* tag) {
    shared_mock->async_write(op);
  }

  /// Intercept an asynchronous Read() operation over a rdwr RPC stream
  template <typename W, typename R, typename op_type>
  void async_read(
      async_rdwr_stream<W, R> const& stream, std::shared_ptr<op_type>& op,
      void* tag) {
    shared_mock->async_read(op);
  }

  /// Intercept an asynchronous WriteDone() operation over a rdwr RPC stream
  template <typename W, typename R, typename op_type>
  void async_writes_done(
      async_rdwr_stream<W, R> const& stream, std::shared_ptr<op_type>& op,
      void* tag) {
    shared_mock->async_writes_done(op);
  }

  /// Intercept an asynchronous Finish() operation over a rdwr RPC stream
  template <typename W, typename R, typename op_type>
  void async_finish(
      async_rdwr_stream<W, R> const& stream, std::shared_ptr<op_type>& op,
      void* tag) {
    shared_mock->async_finish(op);
  }

  struct mocked {
    MOCK_CONST_METHOD1(async_rpc, void(std::shared_ptr<base_async_op> op));
    MOCK_CONST_METHOD1(
        async_create_rdwr_stream, void(std::shared_ptr<base_async_op> op));
    MOCK_CONST_METHOD1(async_write, void(std::shared_ptr<base_async_op> op));
    MOCK_CONST_METHOD1(async_read, void(std::shared_ptr<base_async_op> op));
    MOCK_CONST_METHOD1(
        async_writes_done, void(std::shared_ptr<base_async_op> op));
    MOCK_CONST_METHOD1(async_finish, void(std::shared_ptr<base_async_op> op));
  };

  std::shared_ptr<mocked> shared_mock;
};

} // namespace detail
} // namespace etcd
} // namespace jb

#endif // jb_etcd_detail_mocked_grpc_interceptor_hpp
