#ifndef jb_etcd_client_factory_hpp
#define jb_etcd_client_factory_hpp

#include <etcd/etcdserver/etcdserverpb/rpc.grpc.pb.h>
#include <grpc++/grpc++.h>

namespace jb {
namespace etcd {

// TODO() - this is pretty raw, the returned interfaces do not hide
// anything.  Eventually they will, as I introduce a Fake of each to
// implement unit tests.
class client_factory {
public:
  client_factory();
  virtual ~client_factory();

  virtual std::shared_ptr<grpc::Channel>
  create_channel(std::string const& etcd_endpoint) {
    return std::shared_ptr<grpc::Channel>(
        grpc::CreateChannel(etcd_endpoint, grpc::InsecureChannelCredentials()));
  }

  virtual std::unique_ptr<etcdserverpb::KV::Stub>
  create_kv(std::shared_ptr<grpc::Channel> channel) {
    return etcdserverpb::KV::NewStub(channel);
  }

  virtual std::unique_ptr<etcdserverpb::Watch::Stub>
  create_watch(std::shared_ptr<grpc::Channel> channel) {
    return etcdserverpb::Watch::NewStub(channel);
  }

  virtual std::unique_ptr<etcdserverpb::Lease::Stub>
  create_lease(std::shared_ptr<grpc::Channel> channel) {
    return etcdserverpb::Lease::NewStub(channel);
  }
};

} // namespace etcd
} // namespace jb

#endif // jb_etcd_client_factory_hpp
