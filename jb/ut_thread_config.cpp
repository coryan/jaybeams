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
