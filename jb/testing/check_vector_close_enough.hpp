#ifndef jb_testing_check_vector_close_enough_hpp
#define jb_testing_check_vector_close_enough_hpp

#include <jb/testing/check_complex_close_enough.hpp>

namespace jb {
namespace testing {

/**
 * Report (using Boost.Test functions) any differences between the
 * elements in @a actual vs. @a expected.
 *
 * Given two collections of floating point or complex numbers find the
 * differences and report them via Boost.Test functions.
 */
template <typename vector>
int check_vector_close_enough(
    vector const& actual, vector const& expected, int tol = 1,
    int max_differences = JB_TESTING_MAX_DIFFERENCES) {
  if (max_differences <= 0) {
    return 0;
  }
  BOOST_CHECK_EQUAL(actual.size(), expected.size());
  if (actual.size() != expected.size()) {
    return static_cast<int>(std::max(actual.size(), expected.size()));
  }

  int count = 0;
  for (std::size_t i = 0; i != actual.size(); ++i) {
    if (close_enough(actual[i], expected[i], tol)) {
      continue;
    }
    BOOST_TEST_MESSAGE(
        "in item i=" << i << " difference higher than tolerance=" << tol
                     << ", actual[i]=" << actual[i]
                     << ", expected[i]=" << expected[i]);
    if (++count > max_differences) {
      return count;
    }
  }
  return count;
}

} // namespace testing
} // namespace jb

#endif // jb_testing_check_vector_close_enough_hpp
