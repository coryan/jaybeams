#include "jb/etcd/leader_election_participant.hpp"

#include <boost/test/unit_test.hpp>
#include <atomic>
#include <thread>

/**
 * @test Verify that one can create, run, and stop a completion queue.
 */
BOOST_AUTO_TEST_CASE(leader_election_participant_basic) {
  auto factory = std::make_shared<jb::etcd::client_factory>();
  jb::etcd::leader_election_participant tested(
      factory, "localhost:2379", "testing-election", "42");
}
