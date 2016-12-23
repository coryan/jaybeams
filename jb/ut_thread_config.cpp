#include <jb/thread_config.hpp>

#include <boost/test/unit_test.hpp>

#include <sched.h>

/**
 * @test Verify that basic functionality works as expected.
 */
BOOST_AUTO_TEST_CASE(thread_config_basic) {
  jb::thread_config tested;

  BOOST_CHECK_EQUAL(tested.name(), "");
  BOOST_CHECK_NO_THROW(tested.native_scheduling_policy());
  BOOST_CHECK_NO_THROW(tested.native_priority());

  tested.scheduler("FIFO").priority("MAX");
  BOOST_CHECK_EQUAL(tested.native_scheduling_policy(), SCHED_FIFO);
  BOOST_CHECK_EQUAL(
      tested.native_priority(), sched_get_priority_max(SCHED_FIFO));
}

/**
 * @test Verify that setting the scheduling policy works as expected
 */
BOOST_AUTO_TEST_CASE(thread_config_scheduling_policy) {
  jb::thread_config tested;
  tested.scheduler("RR");
  BOOST_CHECK_EQUAL(tested.native_scheduling_policy(), SCHED_RR);
  tested.scheduler("BATCH");
  BOOST_CHECK_EQUAL(tested.native_scheduling_policy(), SCHED_BATCH);
  tested.scheduler("IDLE");
  BOOST_CHECK_EQUAL(tested.native_scheduling_policy(), SCHED_IDLE);
  tested.scheduler("__not_a_scheduler__");
  BOOST_CHECK_THROW(tested.native_scheduling_policy(), std::exception);
}

/**
 * @test Verify that setting the scheduling priority works as expected
 */
BOOST_AUTO_TEST_CASE(thread_config_scheduling_priority) {
  jb::thread_config tested;

  tested.scheduler("FIFO").priority("MIN");
  BOOST_CHECK_EQUAL(
      tested.native_priority(), sched_get_priority_min(SCHED_FIFO));
  tested.priority("MID");
  BOOST_CHECK_LE(tested.native_priority(), sched_get_priority_max(SCHED_FIFO));
  BOOST_CHECK_GE(tested.native_priority(), sched_get_priority_min(SCHED_FIFO));
  tested.priority("75");
  BOOST_CHECK_EQUAL(tested.native_priority(), 75);
  tested.priority("__not_a_number__");
  BOOST_CHECK_THROW(tested.native_priority(), std::exception);
}

/**
 * @test Verify that parsing YAML overrides works as expected.
 */
BOOST_AUTO_TEST_CASE(thread_config_overrides) {
  jb::thread_config tested;

  char const contents[] = R"""(# YAML overrides
name: foo
scheduler: FIFO
priority: MAX
affinity: 1-3,7
)""";

  std::istringstream is(contents);

  int argc = 0;
  tested.load_overrides(argc, nullptr, is);
  BOOST_CHECK_EQUAL(tested.native_scheduling_policy(), SCHED_FIFO);
  BOOST_CHECK_EQUAL(
      tested.native_priority(), sched_get_priority_max(SCHED_FIFO));

  jb::cpu_set cpus;
  cpus.set(1, 3).set(7);
  BOOST_CHECK_EQUAL(tested.affinity(), cpus);
}
