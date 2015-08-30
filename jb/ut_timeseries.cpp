#include <jb/testing/create_triangle_timeseries.hpp>

#include <boost/test/unit_test.hpp>

/**
 * @test Verify jb::timeseries can be used as expected.
 *
 * This is a very simple class, but the interface is defined by a
 * series of using declarations.  We want to make sure we did not miss
 * any important ones.
 */
BOOST_AUTO_TEST_CASE(create_triangle_timeseries_default) {
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
