#include "jb/etcd/session.hpp"

#include <boost/test/unit_test.hpp>
#include <atomic>
#include <thread>

namespace jb {
namespace etcd {
// Inject a streaming operator into the namespace because Boost.Test
// needs them.
std::ostream& operator<<(std::ostream& os, session::state s) {
  return os << static_cast<int>(s);
}
}
}

/**
 * @test Verify that one can create and destroy a session.
 */
BOOST_AUTO_TEST_CASE(completion_queue_basic) {
  std::string const address = "localhost:2379";
  std::shared_ptr<grpc::Channel> channel =
      grpc::CreateChannel(address, grpc::InsecureChannelCredentials());

  jb::etcd::session session(channel, std::chrono::seconds(5));
  BOOST_CHECK_NE(session.lease_id(), 0);
}

/**
 * @test Verify that one can create, run, and stop a completion queue.
 */
BOOST_AUTO_TEST_CASE(completion_queue_normal) {
  std::string const address = "localhost:2379";
  std::shared_ptr<grpc::Channel> channel =
      grpc::CreateChannel(address, grpc::InsecureChannelCredentials());

  jb::etcd::session session(channel, std::chrono::seconds(5));
  std::thread t([&session]() { session.run(); });
  BOOST_CHECK_NE(session.lease_id(), 0);

  using state = jb::etcd::session::state;
  for (int i = 0; i != 100 and session.current_state() != state::connected;
       ++i) {
    std::this_thread::sleep_for(std::chrono::milliseconds(2));
  }
  BOOST_CHECK_EQUAL(session.current_state(), state::connected);

  session.revoke();
  t.join();
  BOOST_CHECK_EQUAL(session.current_state(), state::shutdown);
}
