#ifndef jb_testing_check_equal_vector_hpp
#define jb_testing_check_equal_vector_hpp

#include <boost/test/unit_test.hpp>
#include <complex>
#include <cmath>
#include <limits>

namespace jb {
namespace testing {

/**
 * Compare floating point values with some tolerance.
 *
 * @return true if the numbers are within tolerance.
 *
 * @param a the first number to compare
 * @param b the second number to compare
 * @param tol expressed in "number of epsilons", i.e., the function
 *   will tolerate errors up to tol * std::numeric_limit<real>::epsilon()
 * @tparam real the type of floating point number (float, double, long double)
 */
template<typename real>
bool close_enough(real actual, real expected, int tol) {
  real const eps = tol * std::numeric_limits<real>::epsilon();
  if (std::abs(expected) < eps) {
    return std::abs(actual) < eps;
  }
  return std::abs((actual - expected) / expected) < eps;
}

/**
 * Compare complex number values with some tolerance.
 *
 * @return true if the both the real and imaginary parts are within
 * tolerance according to jb::testing::close_enough().
 *
 * @param actual the actual value
 * @param expected the value @a actual is compared against.
 * @param tol expressed in "number of epsilons", i.e., the function
 *   will tolerate errors up to tol * std::numeric_limit<real>::epsilon()
 * @tparam real the type of floating point number (float, double, long double)
 */
template<typename real>
bool close_enough(
    std::complex<real> const& actual, std::complex<real> const& expected,
    int tol = 1) {
  return (close_enough(real(actual), real(expected), tol)
          and close_enough(imag(actual), imag(expected), tol));
}

/**
 * Compare complex number (as arrays of size 2) values with some
 * tolerance.
 *
 * @return true if the both the real and imaginary parts are within
 * tolerance according to jb::testing::close_enough().
 *
 * @param actual the actual value
 * @param expected the value @a actual is compared against.
 * @param tol expressed in "number of epsilons", i.e., the function
 *   will tolerate errors up to tol * std::numeric_limit<real>::epsilon()
 * @tparam real the type of floating point number (float, double, long double)
 */
template<typename real>
bool close_enough(real actual[2], real expected[2], int tol = 1) {
  return (close_enough(actual[0], expected[0], tol)
          and close_enough(actual[0], expected[0], tol));
}

#ifndef JB_TESTING_MAX_DIFFERENCES
#define JB_TESTING_MAX_DIFFERENCES 8
#endif

/**
 * Report (using Boost.Test functions) any differences between the
 * elements in @a actual vs. @a expected.
 *
 * Given two collections of floating point or complex numbers find the
 * differences and report them via Boost.Test functions.
 */
template<typename vector>
void check_vector_close_enough(
    vector const& actual, vector const& expected,
    int tol = 1, int max_differences = JB_TESTING_MAX_DIFFERENCES) {
  BOOST_CHECK_EQUAL(actual.size(), expected.size());
  if (actual.size() != expected.size()) {
    return;
  }

  int count = 0;
  for (std::size_t i = 0; i != actual.size(); ++i) {
    if (close_enough(actual[i], expected[i], tol)) {
      continue;
    }
    BOOST_CHECK_MESSAGE(
        close_enough(actual[i], expected[i], tol),
        "in item i=" << i << " difference higher than tolerance=" << tol
        << ", actual[i]=" << actual[i]
        << ", expected[i]=" << expected[i]);
    if (++count > max_differences) {
      return;
    }
  }
}

template<typename value_type>
value_type format(value_type t) {
  return t;
}

template<typename precision_t>
std::complex<precision_t> format(precision_t v[2]) {
  return std::complex<precision_t>(v[0], v[1]);
}

/**
 * Report (using Boost.Test functions) any differences between the
 * elements in @a actual vs. @a expected.
 *
 * Given two collections of floating point or complex numbers find the
 * differences and report them via Boost.Test functions.
 */
template<typename value_type>
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


#endif // jb_testing_check_equal_vector_hpp
