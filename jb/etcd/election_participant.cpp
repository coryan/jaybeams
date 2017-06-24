#include <jb/etcd/leader_election_participant.hpp>
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
  jb::config_attribute<config, std::string> election_name;
  jb::config_attribute<config, std::string> value;
};
} // anonymous namespace

int main(int argc, char* argv[]) try {
  // Do the usual thin in JayBeams to load the configuration ...
  config cfg;
  cfg.load_overrides(
      argc, argv, std::string("election_listener.yaml"), "JB_ROOT");

  // TODO() - use the default credentials when possible, should be
  // controlled by a configuration parameter ...
  auto factory = std::make_shared<jb::etcd::client_factory>();

  // ... create an active completion queue ...
  // ... to run multiple things asynchronously in gRPC++ we need a
  // completion queue.  Unfortunately cannot share this with
  // Boost.ASIO queue.  Which is a shame, so run a separate thread...
  // TODO() - the thread should be configurable, and use
  // jb::thread_launcher.
  auto queue = std::make_shared<jb::etcd::active_completion_queue>();

  // ... a promise that tells us if this participant has been elected
  // the leader ...
  std::promise<bool> is_leader;

  // ... the election participant, set the is_leader promise when this
  // participant is elected leader ...
  jb::etcd::leader_election_participant participant(
      queue, factory, cfg.etcd_address(), cfg.election_name(), cfg.value(),
      [&is_leader](std::future<bool>& f) {
        try {
          std::cout << "... elected! ..." << std::endl;
          is_leader.set_value(f.get());
        } catch (...) {
          is_leader.set_exception(std::current_exception());
          std::cout << "... election failed ..." << std::endl;
        }
      },
      // TODO() - make the initial TTL configurable.
      std::chrono::seconds(10));

  auto status = is_leader.get_future().wait_for(std::chrono::seconds(0));
  if (status != std::future_status::ready) {
    std::cout << "Waiting until " << participant.key() << " becomes the leader"
              << std::endl;
  } else {
    std::cout << "Participant " << participant.key() << " is the leader"
              << std::endl;
  }

  // ... use Boost.ASIO to wait for a signal, then gracefully shutdown
  // the participant ...
  // TODO() - well, the code never gets here unless we win the
  // election, it would be nice to handle the signal even if the
  // partipant has not won yet.  That would require some more
  // complicated threading, not going there yet ...
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

  // ... resign as the leader, or abandon the attempt to become the
  // leader if not elected yet ...
  participant.resign();

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
          defaults::etcd_address)
    , election_name(
          desc("election-name").help("The name of the election."), this)
    , value(
          desc("value").help("The value published by this participant."),
          this) {
}

void config::validate() const {
  if (election_name() == "") {
    throw jb::usage("Missing --election-name option.", 1);
  }
  if (value() == "") {
    throw jb::usage("Missing --value option.", 1);
  }
  if (etcd_address() == "") {
    throw jb::usage("The etcd-address option cannot be empty.", 1);
  }
}
} // anonymous namespace
