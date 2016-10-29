#ifndef jb_testing_check_array_close_enough_hpp
#define jb_testing_check_array_close_enough_hpp

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
template <typename value_type>
void check_array_close_enough(
    std::size_t size, value_type const* actual, value_type const* expected,
    int tol = 1, int max_differences = JB_TESTING_MAX_DIFFERENCES) {
  int count = 0;
  for (std::size_t i = 0; i != size; ++i) {
    if (close_enough(actual[i], expected[i], tol)) {
      continue;
    }
    if (++count <= max_differences) {
      BOOST_CHECK_MESSAGE(
          close_enough(actual[i], expected[i], tol),
          "in item i=" << i << " difference higher than tolerance=" << tol
                       << ", actual[i]=" << format(actual[i])
                       << ", expected[i]=" << format(expected[i]));
    }
  }
  BOOST_CHECK_MESSAGE(count == 0, "found " << count << " differences");
}

} // namespace testing
} // namespace jb

#endif // jb_testing_check_array_close_enough_hpp
