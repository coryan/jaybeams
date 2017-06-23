#include <jb/etcd/session.hpp>
#include <jb/assert_throw.hpp>
#include <jb/config_object.hpp>
#include <jb/log.hpp>

#include <boost/asio.hpp>
#include <etcd/etcdserver/etcdserverpb/rpc.grpc.pb.h>
#include <grpc++/alarm.h>
#include <grpc++/grpc++.h>

#include <iomanip>
#include <iostream>
#include <mutex>
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

  // ... a session is the JayBeams abstraction to hold a etcd lease ...
  // TODO() - make the initial TTL configurable.
  // TODO() - decide if storing the TTL in milliseconds makes any
  // sense, after all etcd uses seconds ...
  jb::etcd::session session(channel, std::chrono::seconds(10));

  // ... to run multiple things asynchronously in gRPC++ we need a
  // completion queue.  Unfortunately cannot share this with
  // Boost.ASIO queue.  Which is a shame, so run a separate thread...
  // TODO() - the thread should be configurable, and use
  // jb::thread_launcher.
  std::thread t([&session]() { session.run(); });

  // ... use Boost.ASIO to wait for a signal, then gracefully shutdown
  // the session ...
  boost::asio::io_service io;
  boost::asio::signal_set signals(io, SIGINT, SIGTERM);
#if defined(SIGQUIT)
  signals.add(SIGQUIT);
#endif // defined(SIGQUIT)
  signals.async_wait([&io](boost::system::error_code const& ec, int signo) {
    JB_LOG(info) << "Boost.Asio loop terminated by signal [" << signo << "]\n";
    io.stop();
  });

  // ... run until the signal is delivered ...
  io.run();

  // ... gracefully terminate the session ...
  session.initiate_shutdown();
  t.join();

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
