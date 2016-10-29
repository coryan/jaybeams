#include <jb/offline_feed_statistics.hpp>

#include <boost/test/unit_test.hpp>

/**
 * @test Verify that jb::offline_feed_statistics works as expected.
 */
BOOST_AUTO_TEST_CASE(offline_feed_statististics_simple) {
  jb::offline_feed_statistics::config cfg;
  jb::offline_feed_statistics stats(cfg);

  stats.sample(std::chrono::seconds(1), std::chrono::microseconds(1));
  stats.sample(std::chrono::seconds(1) + std::chrono::microseconds(1),
               std::chrono::microseconds(1));
  stats.sample(std::chrono::seconds(1) + std::chrono::microseconds(2),
               std::chrono::microseconds(1));
  stats.sample(std::chrono::seconds(1) + std::chrono::microseconds(3),
               std::chrono::microseconds(1));

  stats.sample(std::chrono::seconds(601) + std::chrono::microseconds(1),
               std::chrono::microseconds(2));
}

/**
 * @test Test jb::offline_feed_statistics csv output.
 */
BOOST_AUTO_TEST_CASE(offline_feed_statististics_print_csv) {
  jb::offline_feed_statistics::config cfg;
  jb::offline_feed_statistics stats(cfg);
  std::ostringstream header;
  stats.print_csv_header(header);
  BOOST_CHECK_EQUAL(header.str().substr(0, 5), std::string("Name,"));

  std::string h = header.str();
  int nheaders = std::count(h.begin(), h.end(), ',');

  std::ostringstream body;
  stats.print_csv("testing", body);
  BOOST_CHECK_EQUAL(body.str().substr(0, 10), std::string("testing,0,"));
  std::string b = body.str();
  int nfields = std::count(b.begin(), b.end(), ',');
  BOOST_CHECK_EQUAL(nfields, nheaders);

  stats.sample(std::chrono::seconds(600), std::chrono::microseconds(2));
  stats.sample(std::chrono::seconds(601), std::chrono::microseconds(2));
  stats.sample(std::chrono::seconds(602), std::chrono::microseconds(2));
  stats.sample(std::chrono::seconds(603), std::chrono::microseconds(2));

  body.str("");
  stats.print_csv("testing", body);
  BOOST_CHECK_EQUAL(body.str().substr(0, 10), std::string("testing,4,"));
  b = body.str();
  nfields = std::count(b.begin(), b.end(), ',');
  BOOST_CHECK_EQUAL(nfields, nheaders);

  BOOST_TEST_MESSAGE("CSV Output for inspection: \n" << h << b);
}

/**
 * @test Verify that jb::offline_feed_statistics::config works as expected.
 */
BOOST_AUTO_TEST_CASE(offline_feed_statististics_config_simple) {
  typedef jb::offline_feed_statistics::config config;

  BOOST_CHECK_NO_THROW(config().validate());
  BOOST_CHECK_THROW(config().max_messages_per_second(-7).validate(), jb::usage);
  BOOST_CHECK_THROW(config().max_messages_per_millisecond(-7).validate(),
                    jb::usage);
  BOOST_CHECK_THROW(config().max_messages_per_microsecond(-7).validate(),
                    jb::usage);
  BOOST_CHECK_THROW(config().max_interarrival_time_nanoseconds(-7).validate(),
                    jb::usage);
  BOOST_CHECK_THROW(config().max_processing_latency_nanoseconds(-7).validate(),
                    jb::usage);
  BOOST_CHECK_THROW(config().reporting_interval_seconds(-1).validate(),
                    jb::usage);
  BOOST_CHECK_NO_THROW(config().reporting_interval_seconds(0).validate());
}
