#include <jb/offline_feed_statistics.hpp>

#include <boost/test/unit_test.hpp>

/**
 * @test Verity that jb::offline_feed_statistics works as expected.
 */
BOOST_AUTO_TEST_CASE(offline_feed_statististics_simple) {
}

/**
 * @test Verity that jb::offline_feed_statistics::config works as expected.
 */
BOOST_AUTO_TEST_CASE(offline_feed_statististics_config_simple) {
  typedef jb::offline_feed_statistics::config config;

  BOOST_CHECK_NO_THROW(config().validate());
  BOOST_CHECK_THROW(
      config().max_messages_per_second(-7).validate(), jb::usage);
  BOOST_CHECK_THROW(
      config().max_messages_per_millisecond(-7).validate(), jb::usage);
  BOOST_CHECK_THROW(
      config().max_messages_per_microsecond(-7).validate(), jb::usage);
  BOOST_CHECK_THROW(
      config().max_interarrival_time_nanoseconds(-7).validate(), jb::usage);
  BOOST_CHECK_THROW(
      config().max_processing_latency_nanoseconds(-7).validate(), jb::usage);
  BOOST_CHECK_THROW(
      config().reporting_interval_seconds(-1).validate(), jb::usage);
  BOOST_CHECK_NO_THROW(
      config().reporting_interval_seconds(0).validate());
}
