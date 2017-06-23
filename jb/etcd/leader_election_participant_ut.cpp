#include "jb/etcd/leader_election_participant.hpp"
#include <jb/etcd/session.hpp>

#include <boost/test/unit_test.hpp>
#include <atomic>
#include <thread>

/**
 * @test Verify that one can create, run, and stop a completion queue.
 */
BOOST_AUTO_TEST_CASE(leader_election_participant_basic) {
  std::string const etcd_address = "localhost:2379";
  auto factory = std::make_shared<jb::etcd::client_factory>();
  jb::etcd::session session(
      factory->create_channel(etcd_address), std::chrono::milliseconds(15000));
  std::thread t([&session] { session.run(); });
  {
    BOOST_TEST_CHECKPOINT("Session created and thread launched");
    jb::etcd::leader_election_participant tested(
        factory, etcd_address, "testing-election", session.lease_id(), "42");
    BOOST_CHECK_EQUAL(tested.value(), "42");
    BOOST_CHECK_EQUAL(tested.key().substr(0, 17), "testing-election/");
  }
  BOOST_TEST_CHECKPOINT("Shutting down session");
  session.revoke();
  t.join();
}
