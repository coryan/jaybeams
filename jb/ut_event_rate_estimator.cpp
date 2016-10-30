#include <jb/event_rate_estimator.hpp>

#include <boost/test/unit_test.hpp>

/**
 * @test Test default construction and basic operations.
 */
BOOST_AUTO_TEST_CASE(event_rate_estimator_base) {
  jb::event_rate_estimator<> stats(std::chrono::microseconds(100));

  std::chrono::microseconds c(1000000);

  int calls = 0;
  int nsamples = 0;
  std::uint64_t last = 0;
  std::uint64_t repeats = 0;
  auto f = [&](std::uint64_t c, std::uint64_t r) {
    ++calls;
    nsamples += r;
    last = c;
    repeats = r;
  };

  stats.sample(c, f);
  stats.sample(c, f);
  stats.sample(c, f);
  BOOST_CHECK_EQUAL(nsamples, 0);
  stats.sample(++c, f);
  BOOST_CHECK_EQUAL(nsamples, 1);
  BOOST_CHECK_EQUAL(last, 3);
  stats.sample(++c, f);
  BOOST_CHECK_EQUAL(nsamples, 2);
  BOOST_CHECK_EQUAL(last, 4);
  c = std::chrono::microseconds(1000000) + std::chrono::microseconds(99);
  stats.sample(c, f);
  BOOST_CHECK_EQUAL(nsamples, 99);
  BOOST_CHECK_EQUAL(last, 5);
  stats.sample(c, f);
  BOOST_CHECK_EQUAL(nsamples, 99);
  BOOST_CHECK_EQUAL(last, 5);
  stats.sample(++c, f);
  BOOST_CHECK_EQUAL(nsamples, 100);
  BOOST_CHECK_EQUAL(last, 7);
  stats.sample(++c, f);
  BOOST_CHECK_EQUAL(nsamples, 101);
  BOOST_CHECK_EQUAL(last, 5);
}

/**
 * @test Verify that event_rate_estimator optimizes big jumps in time.
 */
BOOST_AUTO_TEST_CASE(event_rate_estimator_jump) {
  std::chrono::microseconds period(100);
  jb::event_rate_estimator<> stats(period);

  std::chrono::microseconds ts(1000000);

  int calls = 0;
  int nsamples = 0;
  std::uint64_t last = 0;
  std::uint64_t repeats = 0;
  auto f = [&](std::uint64_t c, std::uint64_t r) {
    ++calls;
    nsamples += r;
    last = c;
    repeats = r;
  };

  // Record one sample ...
  stats.sample(ts, f);
  BOOST_CHECK_EQUAL(calls, 0);
  // ... move time one tick ...
  stats.sample(++ts, f);
  BOOST_CHECK_EQUAL(calls, 1);
  BOOST_CHECK_EQUAL(nsamples, 1);
  BOOST_CHECK_EQUAL(last, 1);
  // ... move time another tick ...
  stats.sample(++ts, f);
  BOOST_CHECK_EQUAL(calls, 2);
  BOOST_CHECK_EQUAL(nsamples, 2);
  BOOST_CHECK_EQUAL(last, 2);

  // ... now skip forward 5 periods ..
  ts += 15 * period;
  stats.sample(ts, f);
  // ... we expect 98 calls to clear the period, then a single call to
  // move the period forward 14.98 times ...
  BOOST_CHECK_EQUAL(calls, 2 + 100 + 1);
  // ... we expect 15 * period + 3 samples ...
  BOOST_CHECK_EQUAL(nsamples, 2 + 15 * period.count());
  BOOST_CHECK_EQUAL(last, 0);
  BOOST_CHECK_EQUAL(repeats, 14 * period.count());

  // .. move time forward one interval at a time ...
  stats.sample(++ts, f);
  stats.sample(++ts, f);
  stats.sample(++ts, f);
  stats.sample(++ts, f);
  BOOST_CHECK_EQUAL(calls, 103 + 4);
  BOOST_CHECK_EQUAL(nsamples, 1502 + 4);
  BOOST_CHECK_EQUAL(last, 4);
  BOOST_CHECK_EQUAL(repeats, 1);

  // ... and make a huge jump with some weird additions ...
  ts += 137 * period + std::chrono::microseconds(7);
  stats.sample(ts, f);
  BOOST_CHECK_EQUAL(calls, 107 + 100 + 1);
  BOOST_CHECK_EQUAL(nsamples, 1506 + 137 * period.count() + 7);
  BOOST_CHECK_EQUAL(last, 0);
  BOOST_CHECK_EQUAL(repeats, 136 * period.count() + 7);
}

/**
 * @test Verify that estimators can use a different bucket size.
 */
BOOST_AUTO_TEST_CASE(event_rate_estimator_milliseconds) {
  typedef std::chrono::milliseconds duration_type;
  jb::event_rate_estimator<duration_type> stats(duration_type(100));

  duration_type c(1000);

  int calls = 0;
  int nsamples = 0;
  std::uint64_t last = 0;
  std::uint64_t repeats = 0;
  auto f = [&](std::uint64_t c, std::uint64_t r) {
    ++calls;
    nsamples += r;
    last = c;
    repeats = r;
  };

  stats.sample(c, f);
  stats.sample(c, f);
  stats.sample(c, f);
  BOOST_CHECK_EQUAL(nsamples, 0);
  stats.sample(++c, f);
  BOOST_CHECK_EQUAL(nsamples, 1);
  BOOST_CHECK_EQUAL(last, 3);
  stats.sample(++c, f);
  BOOST_CHECK_EQUAL(nsamples, 2);
  BOOST_CHECK_EQUAL(last, 4);
  c = std::chrono::milliseconds(1000) + std::chrono::milliseconds(99);
  stats.sample(c, f);
  BOOST_CHECK_EQUAL(nsamples, 99);
  BOOST_CHECK_EQUAL(last, 5);
  stats.sample(c, f);
  BOOST_CHECK_EQUAL(nsamples, 99);
  BOOST_CHECK_EQUAL(last, 5);
  stats.sample(++c, f);
  BOOST_CHECK_EQUAL(nsamples, 100);
  BOOST_CHECK_EQUAL(last, 7);
  stats.sample(++c, f);
  BOOST_CHECK_EQUAL(nsamples, 101);
  BOOST_CHECK_EQUAL(last, 5);
}

/**
 * @test Test error cases
 */
BOOST_AUTO_TEST_CASE(event_rate_estimator_errors) {
  using namespace std::chrono;
  typedef jb::event_rate_estimator<std::chrono::seconds> tested;

  BOOST_CHECK_THROW(tested(seconds(10), seconds(20)), std::invalid_argument);
  BOOST_CHECK_THROW(tested(seconds(10), seconds(0)), std::invalid_argument);
  BOOST_CHECK_THROW(tested(seconds(10), seconds(3)), std::invalid_argument);

  typedef std::chrono::duration<std::size_t> ticks;
  typedef jb::event_rate_estimator<ticks> testbig;
  auto big = std::numeric_limits<std::size_t>::max();
  BOOST_CHECK_THROW(testbig(ticks(big), ticks(1)), std::invalid_argument);
}
