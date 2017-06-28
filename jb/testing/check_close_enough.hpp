#ifndef jb_testing_check_close_enough_hpp
#define jb_testing_check_close_enough_hpp

#include <boost/multi_array.hpp>
#include <boost/test/unit_test.hpp>
#include <cmath>
#include <complex>
#include <limits>
#include <type_traits>

namespace jb {
namespace testing {

#ifndef JB_TESTING_MAX_DIFFERENCES_PRINTED
#define JB_TESTING_MAX_DIFFERENCES_PRINTED 8
#endif // JB_TESTING_MAX_DIFFERENCES_PRINTED

/**
 * Given two numbers of the same integer type check if the difference is within
 * a given tolerance.
 *
 * Tolerance is expressed as the difference between any pair of integer numbers.
 *
 * @return true if the numbers are within tolerance.
 *
 * @param num_a the first number to compare
 * @param num_b the second number to compare
 * @param tol tolerance that the pair of numbers have to be within
 * @tparam value_t the type of the integral argument numbers
 */
template <
    typename value_t,
    typename std::enable_if<std::is_integral<value_t>::value>::type* = nullptr>
bool check_close_enough(value_t num_a, value_t num_b, int tol) {
  using value_type = value_t;
  value_type value_tol = static_cast<value_type>(tol);
  if (num_a == num_b) {
    return true;
  }
  if ((num_a < num_b - value_tol) or (num_a > num_b + value_tol)) {
    return false;
  }
  return true;
}

/**
 * Given two numbers of the same floating-point type check if the difference is
 * within
 * a given tolerance.
 *
 * Tolerance is expressed in "number of epsilons" (i.e., the function will
 * tolerate errors up to tolerance * std::numeric_limit<real>::epsilon()
 *
 * @return true if the numbers are within tolerance.
 *
 * @param num_a the first number to compare
 * @param num_b the second number to compare
 * @param tol tolerance that the pair of numbers have to be within
 * @tparam value_t the type of the floating-point argument numbers
 */
template <
    typename value_t,
    typename std::enable_if<std::is_floating_point<value_t>::value>::type* =
        nullptr>
bool check_close_enough(value_t num_a, value_t num_b, int tol) {
  using value_type = value_t;
  value_type const eps = tol * std::numeric_limits<value_type>::epsilon();
  if (std::abs(num_a) <= eps) {
    return std::abs(num_b) <= eps;
  }
  return std::abs((num_a - num_b) / num_b) <= eps;
}

/**
 * Given two complex numbers of the same value type check if the difference is
 * within
 * a given tolerance.
 *
 * If value types are floating-point numbers, tolerance is expressed
 * in "number of epsilons" (i.e., the function will tolerate errors up to
 * tolerance * std::numeric_limit<real>::epsilon()
 *
 * If value types are integer numbers, tolerance is expressed as the
 * difference between any pair of integer numbers.
 *
 * @return true if the both the real and imaginary parts are within
 * tolerance
 *
 * @param com_a the first complex number to compare
 * @param com_b the second complex number to compare
 * @param tol tolerance that the pair of numbers have to be within
 * @tparam value_type the type of std::complex number
 */
template <typename value_t>
bool check_close_enough(
    std::complex<value_t> com_a, std::complex<value_t> com_b, int tol = 1) {
  return (
      check_close_enough(com_a.real(), com_b.real(), tol) and
      check_close_enough(com_a.imag(), com_b.imag(), tol));
}

/**
 * Compare complex number (as arrays of size 2) values with some
 * tolerance.
 *
 * @return true if the both the real and imaginary parts are within
 * tolerance.
 *
 * @param com_a the first complex number to compare
 * @param com_b the second complex number to compare
 * @param tol tolerance that the pair of numbers have to be within
 * @tparam value_type the type of std::complex number
 */
template <typename value_t>
bool check_close_enough(value_t com_a[2], value_t com_b[2], int tol = 1) {
  return (
      check_close_enough(com_a[0], com_b[0], tol) and
      check_close_enough(com_a[1], com_b[1], tol));
}

/**
 * Verify that a floating point value is "close enough" to a small number.
 */
template <typename floating>
void check_small(floating t, double small) {
  BOOST_CHECK_SMALL(t, floating(small));
}

/**
 * Verify that a complex number is "close enough" to a small number.
 */
template <typename real>
void check_small(std::complex<real> t, double small) {
  BOOST_CHECK_SMALL(t.real(), real(small));
  BOOST_CHECK_SMALL(t.imag(), real(small));
}

/// Wrap FFTW-style complex numbers in std::complex for iostreaming
template <typename precision_t>
std::complex<precision_t> format(precision_t v[2]) {
  return std::complex<precision_t>(v[0], v[1]);
}

/// Allow generic treatment of FFTW-style complex numbers and other types.
template <typename value_type>
value_type format(value_type t) {
  return t;
}

/**
 * Calculate the relative error between two float point numbers.
 *
 * @return the relative error of actual compared to offset.
 *
 * @param actual the number the test got
 * @param expected the number the test expected
 *
 * @tparam real the type of floating point number (float, double, long double)
 */
template <
    typename real, typename std::enable_if<
                       std::is_floating_point<real>::value>::type* = nullptr>
real relative_error(real actual, real expected) {
  if (std::numeric_limits<real>::is_integer) {
    return std::abs(actual - expected);
  }
  if (std::abs(expected) < std::numeric_limits<real>::epsilon()) {
    return std::abs(actual);
  }
  return std::abs((actual - expected) / expected);
}

/**
 * Adapt relative_error() for integral numbers.
 *
 * @return the absolute difference.
 *
 * @param actual the number the test got
 * @param expected the number the test expected
 *
 * @tparam integral the type of integral number
 */
template <
    typename integral,
    typename std::enable_if<std::is_integral<integral>::value>::type* = nullptr>
integral relative_error(integral actual, integral expected) {
  if (actual > expected) {
    return actual - expected;
  }
  return expected - actual;
}

/**
 * Calculate the relative error between two complex numbers.
 *
 * @return the relative error of actual compared to offset, using the
 * Manhattan metric.
 *
 * @param actual the number the test got
 * @param expected the number the test expected
 *
 * @tparam real the type of floating point number (float, double, long double)
 */
template <typename real>
real relative_error(std::complex<real> actual, std::complex<real> expected) {
  return std::max(
      relative_error(actual.real(), expected.real()),
      relative_error(actual.imag(), expected.imag()));
}

/**
 * Calculate the relative error between two complex numbers in FFTW
 * representation.
 *
 * @return the relative error of actual compared to offset, using the
 * Manhattan metric.
 *
 * @param actual the number the test got
 * @param expected the number the test expected
 *
 * @tparam real the type of floating point number (float, double, long double)
 */
template <typename real>
real relative_error(real actual[2], real expected[2]) {
  return std::max(
      relative_error(actual[0], expected[0]),
      relative_error(actual[1], expected[1]));
}

/**
 * Given two collections of numbers of the same value type, find the differences
 * that are out of a given tolerance and report them via Boost.Test functions.
 *
 * Collections of numbers are any container representation (e.g. vector, deque),
 * boost::multi_array, c-like array, etc.
 *
 * Value types are integers, floating-point, and std::complex (integers or
 * floating-point).
 *
 * If value types are floating-point numbers, tolerance is expressed
 * in "number of epsilons" (i.e., the function will tolerate errors up to
 * tolerance * std::numeric_limit<real>::epsilon()
 *
 * If value types are integer numbers, tolerance is expressed as the
 * difference between any pair of integer numbers.
 *
 * @return true if the numbers are within tolerance.
 *
 * @param a the first collection of numbers to compare
 * @param b the second collection of numbers to compare
 * @param tol tolerance that each pair of numbers have to be within
 * @param max_differences_printed how many differences will be printed
 *   out in full detail, some of the collections are large and printing
 *   all the differences can be overwhelming
 * @tparam collection_t the type of the collection containing the numbers
 *   (e.g. std::vector<>, boost::multi_array)
 */
template <typename collection_t>
bool check_collection_close_enough(
    collection_t const& a, collection_t const& b, int tol = 1,
    int max_differences_printed = JB_TESTING_MAX_DIFFERENCES_PRINTED) {
  BOOST_CHECK_EQUAL(a.size(), b.size());
  if (a.size() != b.size()) {
    return false;
  }

  int count = 0;
  for (std::size_t i = 0; i != a.size(); ++i) {
    if (check_close_enough(a[i], b[i], tol)) {
      continue;
    }
    if (++count <= max_differences_printed) {
      auto error = relative_error(a[i], b[i]);
      BOOST_ERROR(
          "in item i=" << i << " difference higher than tolerance=" << tol
                       << ", actual[i]=" << format(a[i]) << ", expected[i]="
                       << format(b[i]) << ", error=" << error);
    }
  }
  return count == 0;
}

/**
 * Specialization for boost::multi_array type
 *
 * @return true if the numbers are within tolerance.
 *
 * @param a the first multi array of numbers to compare
 * @param b the second multi array of numbers to compare
 * @param tol tolerance that each pair of numbers have to be within
 * @param max_differences_printed how many differences will be printed
 *   out in full detail, some of the collections are large and printing
 *   all the differences can be overwhelming
 * @tparam T type of the value stored on the timeseries
 * @tparam K timeseries dimensionality
 * @tparam A an Allocator type for type T allocator storage
 */
template <typename T, std::size_t K>
bool check_collection_close_enough(
    boost::multi_array<T, K> const& a, boost::multi_array<T, K> const& b,
    int tol = 1,
    int max_differences_printed = JB_TESTING_MAX_DIFFERENCES_PRINTED) {
  BOOST_CHECK_EQUAL(a.num_elements(), b.num_elements());
  if (a.num_elements() != b.num_elements()) {
    return false;
  }

  int count = 0;
  for (std::size_t i = 0; i != a.num_elements(); ++i) {
    if (check_close_enough(a.data()[i], b.data()[i], tol)) {
      continue;
    }
    if (++count <= max_differences_printed) {
      auto error = relative_error(a.data()[i], b.data()[i]);
      BOOST_ERROR(
          "in item i=" << i << " difference higher than tolerance=" << tol
                       << ", actual[i]=" << format(a.data()[i])
                       << ", expected[i]=" << format(b.data()[i])
                       << ", error=" << error);
    }
  }
  return count == 0;
}

/**
 * Given two collections of integers, floating point or complex numbers
 * find the differences that are out of a given tolerance and report them
 * via Boost.Test functions.
 *
 * @return true if the numbers are within tolerance.
 *
 * @param size the size of collection
 * @param a the first multi array of numbers to compare
 * @param b the second multi array of numbers to compare
 * @param tol tolerance that each pair of numbers have to be within
 * @param max_differences_printed how many differences will be printed
 *   out in full detail, some of the collections are large and printing
 *   all the differences can be overwhelming
 * @tparam value_t value type of the number stored on collections
 */
template <typename value_t>
bool check_collection_close_enough(
    std::size_t size, value_t const* a, value_t const* b, int tol = 1,
    int max_differences_printed = JB_TESTING_MAX_DIFFERENCES_PRINTED) {
  int count = 0;
  for (std::size_t i = 0; i != size; ++i) {
    if (check_close_enough(a[i], b[i], tol)) {
      continue;
    }
    if (++count <= max_differences_printed) {
      auto error = relative_error(a[i], b[i]);
      BOOST_ERROR(
          "in item i=" << i << " difference higher than tolerance=" << tol
                       << ", a[i]=" << format(a[i]) << ", b[i]=" << format(b[i])
                       << ", error=" << error);
    }
  }
  return count == 0;
}

} // namespace testing
} // namespace jb

#endif // jb_testing_check_close_enough_hpp
