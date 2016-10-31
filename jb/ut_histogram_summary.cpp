#include <jb/histogram_summary.hpp>

#include <boost/test/unit_test.hpp>

/**
 * @test Verify that a histogram_summary works as expected.
 */
BOOST_AUTO_TEST_CASE(histogram_summary_ios) {
  jb::histogram_summary summary{0, 25, 50, 75, 90, 99, 100, 1000};
  std::ostringstream os;
  os << summary;
  BOOST_CHECK_EQUAL(
      os.str(), "nsamples=1000, min=0, p25=25, "
                "p50=50, p75=75, p90=90, max=100");
}
