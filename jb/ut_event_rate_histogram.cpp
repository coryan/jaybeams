#include <jb/event_rate_histogram.hpp>

#include <boost/test/unit_test.hpp>

/**
 * @test Test the basic functionality of a event_rate_histogram.
 */
BOOST_AUTO_TEST_CASE(event_rate_histogram_basics) {
  typedef jb::event_rate_histogram<> tested_class;

  tested_class t(1000, std::chrono::microseconds(1000));
  BOOST_CHECK_EQUAL(t.nsamples(), 0);
  BOOST_CHECK_THROW(t.last_rate(), std::invalid_argument);
  BOOST_CHECK_THROW(t.estimated_mean(), std::invalid_argument);

  t.sample(std::chrono::microseconds(10));
  t.sample(std::chrono::microseconds(10));
  t.sample(std::chrono::microseconds(11));
  BOOST_CHECK_EQUAL(t.nsamples(), 1);
  BOOST_CHECK_EQUAL(t.last_rate(), 2);
  BOOST_CHECK_EQUAL(t.observed_max(), 2);
}

/**
 * @test Test that even rate histograms capture repeated elements properly.
 */
BOOST_AUTO_TEST_CASE(event_rate_histogram_repeats) {
  typedef jb::event_rate_histogram<> tested_class;

  tested_class t(1000, std::chrono::microseconds(1000));
  BOOST_CHECK_EQUAL(t.nsamples(), 0);
  BOOST_CHECK_THROW(t.last_rate(), std::invalid_argument);
  BOOST_CHECK_THROW(t.estimated_mean(), std::invalid_argument);

  t.sample(std::chrono::microseconds(10));
  t.sample(std::chrono::microseconds(11));
  t.sample(std::chrono::microseconds(12));
  BOOST_CHECK_EQUAL(t.nsamples(), 2);
  BOOST_CHECK_EQUAL(t.last_rate(), 2);
  BOOST_CHECK_EQUAL(t.observed_max(), 2);
  t.sample(std::chrono::microseconds(1012));
  BOOST_CHECK_EQUAL(t.nsamples(), 1002);
  BOOST_CHECK_EQUAL(t.last_rate(), 1);
  BOOST_CHECK_EQUAL(t.observed_max(), 3);
  t.sample(std::chrono::microseconds(5012));
  BOOST_CHECK_EQUAL(t.nsamples(), 5002);
  BOOST_CHECK_EQUAL(t.last_rate(), 0);
  BOOST_CHECK_EQUAL(t.observed_max(), 3);
}
