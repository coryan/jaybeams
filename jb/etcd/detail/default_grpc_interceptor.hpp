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
  /// Post an asynchronous RPC operation via the completion queue
  template <typename C, typename M, typename op_type>
  void async_rpc(
      C* async_client, M C::*call, std::shared_ptr<op_type> op,
      grpc::CompletionQueue* cq, void* tag) {
    op->rpc = (async_client->*call)(&op->context, op->request, cq);
    op->rpc->Finish(&op->response, &op->status, tag);
  }

  /// Post an asynchronous operation to create a rdwr RPC stream
  template <typename C, typename M, typename op_type>
  void async_create_rdwr_stream(
      C* async_client, M C::*call, std::shared_ptr<op_type> op,
      grpc::CompletionQueue* cq, void* tag) {
    op->stream->client = (async_client->*call)(&op->stream->context, cq, tag);
  }

  /// Post an asynchronous Write() operation over a rdwr RPC stream
  template <typename W, typename R, typename op_type>
  void async_write(
      async_rdwr_stream<W, R> const& stream, std::shared_ptr<op_type> op,
      void* tag) {
    stream.client->Write(op->request, tag);
  }

  /// Post an asynchronous Read() operation over a rdwr RPC stream
  template <typename W, typename R, typename op_type>
  void async_read(
      async_rdwr_stream<W, R> const& stream, std::shared_ptr<op_type> op,
      void* tag) {
    stream.client->Read(&op->response, tag);
  }

  /// Post an asynchronous WriteDone() operation over a rdwr RPC stream
  template <typename W, typename R, typename op_type>
  void async_writes_done(
      async_rdwr_stream<W, R> const& stream, std::shared_ptr<op_type> op,
      void* tag) {
    stream.client->WritesDone(tag);
  }

  /// Post an asynchronous Finish() operation over a rdwr RPC stream
  template <typename W, typename R, typename op_type>
  void async_finish(
      async_rdwr_stream<W, R> const& stream, std::shared_ptr<op_type> op,
      void* tag) {
    stream.client->Finish(&op->status, tag);
  }
};

} // namespace detail
} // namespace etcd
} // namespace jb

#endif // jb_etcd_detail_default_grpc_interceptor_hpp
