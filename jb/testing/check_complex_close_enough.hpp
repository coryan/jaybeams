#ifndef jb_testing_check_complex_close_enough_hpp
#define jb_testing_check_complex_close_enough_hpp

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
  return (close_enough(actual.real(), expected.real(), tol)
          and close_enough(actual.imag(), expected.imag(), tol));
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

/**
 * Verify that a floating point value is "close enough" to 0.
 */
template<typename floating>
void check_small(floating t, double small) {
  BOOST_CHECK_SMALL(t, floating(small));
}

/**
 * Verify that a complex number is "close enough" to 0.
 */
template<typename real>
void check_small(std::complex<real> t, double small) {
  BOOST_CHECK_SMALL(t.real(), real(small));
  BOOST_CHECK_SMALL(t.imag(), real(small));
}

/// Wrap FFTW-style complex numbers in std::complex for iostreaming
template<typename precision_t>
std::complex<precision_t> format(precision_t v[2]) {
  return std::complex<precision_t>(v[0], v[1]);
}

/// Allow generic treatment of FFTW-style complex numbers and other types.
template<typename value_type>
value_type format(value_type t) {
  return t;
}

#ifndef JB_TESTING_MAX_DIFFERENCES
#define JB_TESTING_MAX_DIFFERENCES 8
#endif

} // namespace testing
} // namespace jb

#endif // jb_testing_check_complex_close_enough_hpp
