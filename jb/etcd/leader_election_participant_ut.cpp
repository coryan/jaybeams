#include "jb/etcd/leader_election_participant.hpp"
#include <jb/log.hpp>
#include <jb/testing/future_status.hpp>

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

  jb::etcd::session session_c(
      queue, factory, etcd_address, std::chrono::milliseconds(3000));
  BOOST_CHECK_NE(session_b.lease_id(), 0);
  std::promise<bool> elected_c;
  jb::etcd::leader_election_participant participant_c(
      queue, factory, etcd_address, election_name, "session_c",
      [&elected_c](std::future<bool>&) { elected_c.set_value(true); },
      std::chrono::seconds(3));
  BOOST_CHECK_EQUAL(participant_c.value(), "session_c");

  std::this_thread::sleep_for(std::chrono::seconds(5));

  auto future_c = elected_c.get_future();
  BOOST_CHECK_EQUAL(
      std::future_status::timeout, future_c.wait_for(std::chrono::seconds(0)));
  auto future_b = elected_b.get_future();
  BOOST_CHECK_EQUAL(
      std::future_status::timeout, future_b.wait_for(std::chrono::seconds(0)));

  for (int i = 0; i != 2; ++i) {
    BOOST_TEST_CHECKPOINT("iteration i=" << i);
    BOOST_CHECK_NO_THROW(participant_a.proclaim("I am the best"));
    BOOST_CHECK_NO_THROW(participant_b.proclaim("No you are not"));
    BOOST_CHECK_NO_THROW(participant_c.proclaim("Both wrong"));
  }

  BOOST_TEST_CHECKPOINT("a::resign");
  participant_a.resign();

  BOOST_CHECK_THROW(participant_a.proclaim("not dead yet"), std::exception);
  try {
    participant_a.proclaim("no really");
  } catch (std::exception const& ex) {
    BOOST_TEST_MESSAGE("exception value: " << ex.what());
  }

  BOOST_TEST_CHECKPOINT("b::resign");
  participant_b.resign();

  BOOST_TEST_CHECKPOINT("c::resign");
  participant_c.resign();

  BOOST_CHECK_EQUAL(future_c.get(), true);

  BOOST_TEST_CHECKPOINT("cleanup");
  election_session.revoke();
}
