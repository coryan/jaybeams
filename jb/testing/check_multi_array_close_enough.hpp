#ifndef jb_testing_check_multi_array_close_enough_hpp
#define jb_testing_check_multi_array_close_enough_hpp

#include <jb/testing/check_vector_close_enough.hpp>

namespace jb {
namespace testing {

// Forward declaration ...
template <typename multi_array, std::size_t K>
struct check_multi_array_close_enough_helper;

/**
 * Report (using Boost.Test functions) any differences between the
 * elements in @a actual vs. @a expected, when the dimensionality is 1.
 *
 * Given two collections of floating point or complex numbers find the
 * differences and report them via Boost.Test functions.
 */
template <typename multi_array>
struct check_multi_array_close_enough_helper<multi_array, 1> {
  static int check(
      multi_array const& actual, multi_array const& expected, int tol,
      int max_differences) {
    if (max_differences < 0) {
      return 0;
    }

    return check_vector_close_enough(actual, expected, tol, max_differences);
  }
};

/**
 * Report (using Boost.Test functions) any differences between the
 * elements in @a actual vs. @a expected, when the dimensionality is 0.
 */
template <typename multi_array>
struct check_multi_array_close_enough_helper<multi_array, 0> {
  static int check(
      multi_array const& actual, multi_array const& expected, int tol,
      int max_differences) {
    return static_cast<int>(not check_close_enough(actual, expected, tol));
  }
};

/**
 * Report (using Boost.Test functions) any differences between the
 * elements in @a actual vs. @a expected, when the dimensionality of
 * the arrays is known.
 *
 * Given two collections of floating point or complex numbers find the
 * differences and report them via Boost.Test functions.
 */
template <typename multi_array, std::size_t K>
struct check_multi_array_close_enough_helper {
  static int check(
      multi_array const& actual, multi_array const& expected, int tol,
      int max_differences) {
    if (max_differences < 0) {
      return 0;
    }

    int mismatches = 0;
    for (typename multi_array::size_type i = 0; i != actual.size(); ++i) {
      typedef typename multi_array::value_type value_type;
      mismatches +=
          check_multi_array_close_enough_helper<value_type, K - 1>::check(
              actual[i], expected[i], tol, max_differences - mismatches);
    }
    return mismatches;
  }
};

/**
 * Report (using Boost.Test functions) any differences between the
 * elements in @a actual vs. @a expected.
 *
 * Given two collections of floating point or complex numbers find the
 * differences and report them via Boost.Test functions.
 */
template <typename multi_array>
int check_multi_array_close_enough(
    multi_array const& actual, multi_array const& expected, int tol = 1,
    int max_differences = JB_TESTING_MAX_DIFFERENCES) {
  return check_multi_array_close_enough_helper<multi_array,
                                               multi_array::dimensionality>::
      check(actual, expected, tol, max_differences);
}

} // namespace testing
} // namespace jb

#endif // jb_testing_check_multi_array_close_enough_hpp
