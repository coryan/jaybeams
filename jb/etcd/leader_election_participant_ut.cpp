#include "jb/etcd/leader_election_participant.hpp"
#include <jb/log.hpp>

#include <boost/test/unit_test.hpp>
#include <atomic>
#include <thread>

/**
 * @test Verify that one can create, and delete a election participant.
 */
BOOST_AUTO_TEST_CASE(leader_election_participant_basic) {
  std::string const etcd_address = "localhost:2379";
  auto factory = std::make_shared<jb::etcd::client_factory>();
  auto queue = std::make_shared<jb::etcd::active_completion_queue>();

  // ... create a unique election name ...
  jb::etcd::session election_session(
      queue, factory, etcd_address, std::chrono::milliseconds(3000));
  BOOST_CHECK_NE(election_session.lease_id(), 0);

  std::ostringstream os;
  os << "test-election/" << std::hex << election_session.lease_id();
  auto election_name = os.str();
  BOOST_TEST_MESSAGE("testing with election-name=" << election_name);

  {
    jb::etcd::leader_election_participant tested(
        queue, factory, etcd_address, election_name, "42",
        [](std::future<bool>&) {}, std::chrono::seconds(3));
    BOOST_TEST_CHECKPOINT("participant object constructed");
    BOOST_CHECK_EQUAL(tested.value(), "42");
    BOOST_CHECK_EQUAL(
        tested.key().substr(0, election_name.size()), election_name);
  }
  BOOST_TEST_MESSAGE("destructed participant, revoking session leases");
  election_session.revoke();
}

/**
 * @test Verify that an election participant can become the leader.
 */
BOOST_AUTO_TEST_CASE(leader_election_participant_switch_leader) {
  std::string const etcd_address = "localhost:2379";
  auto factory = std::make_shared<jb::etcd::client_factory>();
  auto queue = std::make_shared<jb::etcd::active_completion_queue>();
  BOOST_TEST_CHECKPOINT("queue created and thread requested");

  // ... create a unique election name ...
  jb::etcd::session election_session(
      queue, factory, etcd_address, std::chrono::milliseconds(3000));
  BOOST_CHECK_NE(election_session.lease_id(), 0);

  std::ostringstream os;
  os << "test-election/" << std::hex << election_session.lease_id();
  auto election_name = os.str();
  BOOST_TEST_MESSAGE("testing with election-name=" << election_name);

  jb::etcd::leader_election_participant participant_a(
      queue, factory, etcd_address, election_name, "session_a",
      std::chrono::seconds(3));
  BOOST_CHECK_EQUAL(participant_a.value(), "session_a");

  jb::etcd::session session_b(
      queue, factory, etcd_address, std::chrono::milliseconds(3000));
  BOOST_CHECK_NE(session_b.lease_id(), 0);
  std::promise<bool> elected_b;
  jb::etcd::leader_election_participant participant_b(
      queue, factory, etcd_address, election_name, "session_b",
      [&elected_b](std::future<bool>&) { elected_b.set_value(true); },
      std::chrono::seconds(3));
  BOOST_CHECK_EQUAL(participant_b.value(), "session_b");

  std::this_thread::sleep_for(std::chrono::seconds(5));

  BOOST_TEST_CHECKPOINT("a::resign");
  participant_a.resign();

  BOOST_CHECK_EQUAL(elected_b.get_future().get(), true);

  BOOST_TEST_CHECKPOINT("b::resign");
  participant_b.resign();

  BOOST_TEST_CHECKPOINT("cleanup");
  election_session.revoke();
}
