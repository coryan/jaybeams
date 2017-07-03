#include "jb/testing/future_status.hpp"

#include <boost/test/unit_test.hpp>
#include <map>

/**
 * @test Verify that the operators work as expected
 */
BOOST_AUTO_TEST_CASE(jb_future_status_streaming) {
  std::map<std::future_status, std::string> tests = {
      {std::future_status::timeout, "[timeout]"},
      {std::future_status::deferred, "[deferred]"},
      {std::future_status::ready, "[ready]"},
  };
  for (auto const& tcase : tests) {
    std::ostringstream os;
    os << tcase.first;
    BOOST_CHECK_EQUAL(os.str(), tcase.second);
  }
}
