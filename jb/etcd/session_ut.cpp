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
BOOST_AUTO_TEST_CASE(session_basic) {
  std::string const address = "localhost:2379";
  auto etcd_channel =
      grpc::CreateChannel(address, grpc::InsecureChannelCredentials());
  auto queue = std::make_shared<jb::etcd::active_completion_queue>();

  // We want to test that the destructor does not throw, so use a
  // smart pointer ...
  auto session = std::make_unique<jb::etcd::session>(
      queue, etcd_channel, std::chrono::seconds(5));
  BOOST_CHECK_NE(session->lease_id(), 0);
  BOOST_CHECK_NO_THROW(session.reset());
}

/**
 * @test Verify that one can create, run, and stop a completion queue.
 */
BOOST_AUTO_TEST_CASE(session_normal) {
  std::string const address = "localhost:2379";
  auto etcd_channel =
      grpc::CreateChannel(address, grpc::InsecureChannelCredentials());
  auto queue = std::make_shared<jb::etcd::active_completion_queue>();

  jb::etcd::session session(queue, etcd_channel, std::chrono::seconds(5));
  BOOST_CHECK_NE(session.lease_id(), 0);
  using state = jb::etcd::session::state;
  BOOST_CHECK_EQUAL(session.current_state(), state::connected);

  session.revoke();
  BOOST_CHECK_EQUAL(session.current_state(), state::shutdown);
}

/**
 * @test Verify that one can create, run, and stop a completion queue.
 */
BOOST_AUTO_TEST_CASE(session_long) {
  std::string const address = "localhost:2379";
  auto etcd_channel =
      grpc::CreateChannel(address, grpc::InsecureChannelCredentials());
  auto queue = std::make_shared<jb::etcd::active_completion_queue>();

  jb::etcd::session session(queue, etcd_channel, std::chrono::seconds(1));
  BOOST_CHECK_NE(session.lease_id(), 0);
  using state = jb::etcd::session::state;
  BOOST_CHECK_EQUAL(session.current_state(), state::connected);

  // ... keep the session open for a while ...
  std::this_thread::sleep_for(std::chrono::seconds(5));

  session.revoke();
  BOOST_CHECK_EQUAL(session.current_state(), state::shutdown);
}
