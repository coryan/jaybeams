#include <jb/config_object.hpp>

#include <etcd/etcdserver/etcdserverpb/rpc.grpc.pb.h>
#include <grpc++/grpc++.h>
#include <iostream>
#include <stdexcept>

/**
 * Define types and functions used in this program.
 */
namespace {
/// Configuration parameters for moldfeedhandler
class config : public jb::config_object {
public:
  config();
  config_object_constructors(config);

  void validate() const override;

  jb::config_attribute<config, std::string> etcd_address;
};
} // anonymous namespace

int main(int argc, char* argv[]) try {
  // Do the usual thin in JayBeams to load the configuration ...
  config cfg;
  cfg.load_overrides(
      argc, argv, std::string("election_listener.yaml"), "JB_ROOT");

  // TODO() - use the default credentials when possible, should be
  // controlled by a configuration parameter ...
  std::shared_ptr<grpc::Channel> channel = grpc::CreateChannel(
      cfg.etcd_address(), grpc::InsecureChannelCredentials());
  // ... create a stub to communicate to the Watch service ...
  std::unique_ptr<etcdserverpb::Watch::Stub> stub(
      etcdserverpb::Watch::NewStub(channel));

  // ... the Watch API is all streaming, we need a ClientReaderWriter
  // to send and receive messages ...
  grpc::ClientContext context;
  std::unique_ptr<grpc::ClientReaderWriter<
      etcdserverpb::WatchRequest, etcdserverpb::WatchResponse>>
      rdwr(stub->Watch(&context));

  etcdserverpb::WatchRequest req;
  // ... wait for anything starting with "mold", need to make this a
  // parameter of course ... TODO() -
  req.mutable_create_request()->set_key(std::string("mold"));
  req.mutable_create_request()->set_range_end(std::string("mole"));
  req.mutable_create_request()->set_start_revision(0);
  req.mutable_create_request()->set_progress_notify(true);
  req.mutable_create_request()->set_prev_kv(true);
  if (not rdwr->Write(req)) {
    std::cerr << "Write failure " << std::endl;
    return 1;
  }

  std::cout << "WatchRequest sent\n";

  int cnt = 0;
  etcdserverpb::WatchResponse r;
  while (rdwr->Read(&r)) {
    std::cout << "Received response #" << cnt << "\n";
    std::cout << "    header.cluster_id=" << r.header().cluster_id() << "\n"
              << "    header.member_id=" << r.header().member_id() << "\n"
              << "    header.revision=" << r.header().revision() << "\n"
              << "    header.raft_term=" << r.header().raft_term() << "\n"
              << "  created=" << r.created() << "\n"
              << "  canceled=" << r.canceled() << "\n"
              << "  compact_revision=" << r.compact_revision() << "\n"
              << "  cancel_reason=" << r.cancel_reason() << "\n";
    ++cnt;
  }
  auto status = rdwr->Finish();
  if (not status.ok()) {
    std::cerr << "rdwr->Finish() failed: " << status.error_message() << "["
              << status.error_code() << "]" << std::endl;
    return 1;
  }

  return 0;
} catch (jb::usage const& u) {
  std::cerr << u.what() << std::endl;
  return u.exit_status();
} catch (std::exception const& ex) {
  std::cerr << "Standard exception raised: " << ex.what() << std::endl;
  return 1;
} catch (...) {
  std::cerr << "Unknown exception raised" << std::endl;
  return 1;
}

namespace {
namespace defaults {
std::string const etcd_address = "localhost:2379";
} // namespace defaults

config::config()
    : etcd_address(
          desc("etcd-address").help("The address for the etcd server."), this,
          defaults::etcd_address) {
}

void config::validate() const {
}
} // anonymous namespace
