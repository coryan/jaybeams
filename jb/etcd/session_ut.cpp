#include "jb/etcd/session.hpp"
#include <jb/etcd/detail/session_impl.hpp>

#include <boost/test/unit_test.hpp>
#include <atomic>
#include <thread>

/// Define helper types and functions used in these tests
namespace {
using session_type =
    jb::etcd::detail::session_impl<jb::etcd::completion_queue<>>;
} // anonymous namespace

/**
 * @test Verify that one can create and destroy a session.
 */
BOOST_AUTO_TEST_CASE(session_basic) {
  using namespace std::chrono_literals;

  std::string const address = "localhost:2379";
  auto etcd_channel =
      grpc::CreateChannel(address, grpc::InsecureChannelCredentials());
  auto queue = std::make_shared<jb::etcd::active_completion_queue>();

  // We want to test that the destructor does not throw, so use a
  // smart pointer ...
  auto session = std::make_unique<session_type>(
      queue->cq(), etcdserverpb::Lease::NewStub(etcd_channel), 5s);
  BOOST_CHECK_NE(session->lease_id(), 0);
  BOOST_CHECK_NO_THROW(session.reset());
}

/**
 * @test Verify that one can create, run, and stop a completion queue.
 */
BOOST_AUTO_TEST_CASE(session_normal) {
  using namespace std::chrono_literals;

  std::string const address = "localhost:2379";
  auto etcd_channel =
      grpc::CreateChannel(address, grpc::InsecureChannelCredentials());
  auto queue = std::make_shared<jb::etcd::active_completion_queue>();

  session_type session(
      queue->cq(), etcdserverpb::Lease::NewStub(etcd_channel), 5s);
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
  using namespace std::chrono_literals;

  std::string const address = "localhost:2379";
  auto etcd_channel =
      grpc::CreateChannel(address, grpc::InsecureChannelCredentials());
  auto queue = std::make_shared<jb::etcd::active_completion_queue>();

  session_type session(
      queue->cq(), etcdserverpb::Lease::NewStub(etcd_channel), 1s);
  BOOST_CHECK_NE(session.lease_id(), 0);
  using state = jb::etcd::session::state;
  BOOST_CHECK_EQUAL(session.current_state(), state::connected);

  // ... keep the session open for a while ...
  std::this_thread::sleep_for(5000ms);

  session.revoke();
  BOOST_CHECK_EQUAL(session.current_state(), state::shutdown);
}
