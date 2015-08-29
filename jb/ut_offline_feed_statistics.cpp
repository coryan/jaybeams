#include <jb/offline_feed_statistics.hpp>

#include <boost/test/unit_test.hpp>

/**
 * @test Verify that jb::offline_feed_statistics works as expected.
 */
BOOST_AUTO_TEST_CASE(offline_feed_statististics_simple) {
  jb::offline_feed_statistics::config cfg;
  jb::offline_feed_statistics stats(cfg);

  stats.sample(
      std::chrono::seconds(1), std::chrono::microseconds(1));
  stats.sample(
      std::chrono::seconds(1) + std::chrono::microseconds(1),
      std::chrono::microseconds(1));
  stats.sample(
      std::chrono::seconds(1) + std::chrono::microseconds(2),
      std::chrono::microseconds(1));
  stats.sample(
      std::chrono::seconds(1) + std::chrono::microseconds(3),
      std::chrono::microseconds(1));

  stats.sample(
      std::chrono::seconds(601) + std::chrono::microseconds(1),
      std::chrono::microseconds(2));
}

/**
 * @test Verify that jb::offline_feed_statistics works as expected.
 */
BOOST_AUTO_TEST_CASE(offline_feed_statististics_print_empty) {
  jb::offline_feed_statistics::config cfg;
  jb::offline_feed_statistics stats(cfg);
  std::ostringstream header;
  stats.print_csv_header(header);
  BOOST_CHECK_EQUAL(header.str().substr(0, 5), std::string("Name,"));

  std::ostringstream body;
  stats.print_csv("testing", body);
  BOOST_CHECK_EQUAL(
      body.str(), std::string("testing,0"
                              ",,,,,,,,,," // per-sec rate
                              ",,,,,,,,,," // per-msec rate
                              ",,,,,,,,,," // per-usec rate
                              ",,,,,,,,,," // arrival
                              ",,,,,,,,,," // processing latency
                              ));
}

/**
 * @test Verify that jb::offline_feed_statistics::config works as expected.
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
