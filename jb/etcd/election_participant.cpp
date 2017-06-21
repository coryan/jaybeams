#include <jb/config_object.hpp>

#include <etcd/etcdserver/etcdserverpb/rpc.grpc.pb.h>
#include <grpc++/grpc++.h>
#include <iomanip>
#include <iostream>
#include <stdexcept>
#include <thread>

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
  std::unique_ptr<etcdserverpb::Lease::Stub> stub(
      etcdserverpb::Lease::NewStub(channel));

  std::chrono::seconds desired_TTL(1);
  
  // ... request a new lease from etcd(1) ...
  etcdserverpb::LeaseGrantRequest req;
  // ... the TTL is is seconds, convert to the right units ...
  req.set_ttl(
      std::chrono::duration_cast<std::chrono::seconds>(desired_TTL).count());
  // ... let the etcd(1) pick a lease ID ...
  req.set_id(0);

  grpc::ClientContext context;
  etcdserverpb::LeaseGrantResponse resp;
  auto status = stub->LeaseGrant(&context, req, &resp);
  if (not status.ok()) {
    std::cerr << "stub->LeaseGrant() failed: " << status.error_message() << "["
              << status.error_code() << "]" << std::endl;
    return 1;
  }

  if (resp.error() != "") {
    std::cout << "Lease grant request rejected\n"
              << "    header.cluster_id=" << resp.header().cluster_id() << "\n"
              << "    header.member_id=" << resp.header().member_id() << "\n"
              << "    header.revision=" << resp.header().revision() << "\n"
              << "    header.raft_term=" << resp.header().raft_term() << "\n"
              << "  error=" << resp.error();
    // TODO() - probably need to retry until it succeeds, with some
    // kind of backoff, and yes, a really, really long timeout ...
    return 0;
  }
  std::cout << "Lease granted\n"
            << "    header.cluster_id=" << resp.header().cluster_id() << "\n"
            << "    header.member_id=" << resp.header().member_id() << "\n"
            << "    header.revision=" << resp.header().revision() << "\n"
            << "    header.raft_term=" << resp.header().raft_term() << "\n"
            << "  ID=" << std::hex << std::setw(16) << std::setfill('0')
            << resp.id()
            << "  TTL=" << std::dec << resp.ttl() << "s\n";
  auto actual_TTL = std::chrono::duration_cast<std::chrono::milliseconds>(
      std::chrono::seconds(resp.ttl()));

  // ... renew the lease a few times ...
  std::cout << "Keeping lease alive: ";
  grpc::ClientContext kactx;
  std::unique_ptr<grpc::ClientReaderWriter<
      etcdserverpb::LeaseKeepAliveRequest,
      etcdserverpb::LeaseKeepAliveResponse>>
      rdwr(stub->LeaseKeepAlive(&kactx));

  for (int i = 0; i != 10; ++i) {
    etcdserverpb::LeaseKeepAliveRequest ka;
    ka.set_id(resp.id());
    if (not rdwr->Write(ka)) {
      // TODO() - I think this needs to restart connection ...
      std::cerr << "KeepAlive Write failure" << std::endl;
      return 1;
    }
    etcdserverpb::LeaseKeepAliveResponse karesp;
    if (not rdwr->Read(&karesp)) {
      std::cerr << "KeepAlive Read failure" << std::endl;
      return 1;
    }
    std::cout << "." << std::flush;
    actual_TTL = std::chrono::seconds(karesp.ttl());
    std::this_thread::sleep_for(actual_TTL / 5);
  }
  std::cout << "\n"
            << "actual_TTL=" << actual_TTL.count() << "ms\n";
  if (not rdwr->WritesDone()) {
    std::cerr << "Error closing rdwr for leases" << std::endl;
    return 1;
  }
  status = rdwr->Finish();
  if (not status.ok()) {
    std::cerr << "rdwr->Finish() failed: " << status.error_message()
              << "[" << status.error_code() << "]" << std::endl;
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
