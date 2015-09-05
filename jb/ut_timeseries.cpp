#include <jb/timeseries.hpp>

#include <boost/test/unit_test.hpp>
#include <chrono>

/**
 * @test Verify jb::timeseries can be used as expected.
 *
 * This is a very simple class, but the interface is defined by a
 * series of using declarations.  We want to make sure we did not miss
 * any important ones.
 */
BOOST_AUTO_TEST_CASE(timeseries_simple) {
  using std::chrono::milliseconds;
  typedef jb::timeseries<int,milliseconds> tested;

  tested t1(milliseconds(1));
  t1.push_back(1);
  t1.push_back(2);
  t1.push_back(3);
  tested t2(t1);
  tested t3(std::move(t2));

  t2 = t1;
  t3 = std::move(t2);
  tested t4(milliseconds(1), milliseconds(0), {4, 5, 6});
  tested t5(milliseconds(1), milliseconds(0), t4.begin(), t4.end());

  for (int i : t5) {
    t1.push_back(i);
  }

  t1[0] = t3[0];

  BOOST_CHECK_THROW(t1.at(1000), std::exception);
}

/**
 * @test Verify jb::timeseries::extend_by_zeroes can be used as expected.
 */
BOOST_AUTO_TEST_CASE(timeseries_extend_by_zeroes) {
  using std::chrono::milliseconds;
  typedef jb::timeseries<int,milliseconds>::extend_by_zeroes tested;

  tested e;
  auto actual = e(0, 20);
  BOOST_CHECK_EQUAL(actual.first, 0);

  actual = e(-1, 20);
  BOOST_CHECK_EQUAL(actual.first, -1);
  BOOST_CHECK_EQUAL(actual.second, 0);

  actual = e(20, 20);
  BOOST_CHECK_EQUAL(actual.first, -1);
  BOOST_CHECK_EQUAL(actual.second, 0);

  actual = e(21, 20);
  BOOST_CHECK_EQUAL(actual.first, -1);
  BOOST_CHECK_EQUAL(actual.second, 0);

  actual = e(0, 0);
  BOOST_CHECK_EQUAL(actual.first, -1);
  BOOST_CHECK_EQUAL(actual.second, 0);

  actual = e(0, 1);
  BOOST_CHECK_EQUAL(actual.first, 0);
}

/**
 * @test Verify jb::timeseries::extend_by_recycling can be used as expected.
 */
BOOST_AUTO_TEST_CASE(timeseries_extend_by_recycling) {
  using std::chrono::milliseconds;
  typedef jb::timeseries<int,milliseconds>::extend_by_recycling tested;

  tested e;
  auto actual = e(0, 20);
  BOOST_CHECK_EQUAL(actual.first, 0);

  actual = e(-1, 20);
  BOOST_CHECK_EQUAL(actual.first, 19);

  actual = e(20, 20);
  BOOST_CHECK_EQUAL(actual.first, 0);

  actual = e(21, 20);
  BOOST_CHECK_EQUAL(actual.first, 1);

  actual = e(0, 0);
  BOOST_CHECK_EQUAL(actual.first, 0);

  actual = e(0, 1);
  BOOST_CHECK_EQUAL(actual.first, 0);

  actual = e(1, 1);
  BOOST_CHECK_EQUAL(actual.first, 0);

  actual = e(-11, 1);
  BOOST_CHECK_EQUAL(actual.first, 0);
}
