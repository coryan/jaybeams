#include <jb/testing/check_close_enough.hpp>

#include <boost/test/unit_test.hpp>
#include <complex>

/**
 * @test check_close_enough functionality for integer type
 *
 * Usually I do not spend time testing helper functions for tests, but
 * in this case the helper has a lot of template code and we want to
 * validate this before too long.
 */
BOOST_AUTO_TEST_CASE(check_close_enough_integer) {
  using namespace jb::testing;
  using test_type = int;
  int tol = 1;
  test_type a = 10;
  test_type b = 11;
  test_type c = a + 2 * tol;
  BOOST_CHECK_MESSAGE(
      check_close_enough(a, b, tol),
      "a=" << a << ", and b=" << b << " are not within tolerance=" << tol);
  BOOST_CHECK_MESSAGE(
      not check_close_enough(a, c, tol),
      "a=" << a << ", and c=" << c << " are within tolerance=" << tol);
}

/**
 * @test check_close_enough functionality for float type
 */
BOOST_AUTO_TEST_CASE(check_close_enough_float) {
  using namespace jb::testing;
  using test_type = float;
  int tol = 3;
  test_type a = 10.00;
  test_type b = a + std::numeric_limits<test_type>::epsilon();
  test_type c = a + 10 * tol * std::numeric_limits<test_type>::epsilon();
  BOOST_CHECK_MESSAGE(
      check_close_enough(a, b, tol),
      "a=" << a << ", and b=" << b << " are not within tolerance=" << tol);
  BOOST_CHECK_MESSAGE(
      not check_close_enough(a, c, tol),
      "a=" << a << ", and c=" << c << " are within tolerance=" << tol);
}

/**
 * @test check_close_enough functionality for double type
 */
BOOST_AUTO_TEST_CASE(check_close_enough_double) {
  using namespace jb::testing;
  using test_type = double;
  int tol = 3;
  test_type a = 10.00;
  test_type b = a + std::numeric_limits<test_type>::epsilon();
  test_type c = a + 10 * tol * std::numeric_limits<test_type>::epsilon();
  BOOST_CHECK_MESSAGE(
      check_close_enough(a, b, tol),
      "a=" << a << ", and b=" << b << " are not within tolerance=" << tol);
  BOOST_CHECK_MESSAGE(
      not check_close_enough(a, c, tol),
      "a=" << a << ", and c=" << c << " are within tolerance=" << tol);
}

/**
 * @test check_close_enough functionality for complex of integer type
 */
BOOST_AUTO_TEST_CASE(check_close_enough_complex_integer) {
  using namespace jb::testing;
  using value_type = int;
  using test_type = std::complex<value_type>;
  int tol = 3;
  test_type a = {10, 5};
  test_type com_eps = {tol, tol};
  test_type b = a + com_eps;
  test_type c = 2 * com_eps + a;
  BOOST_CHECK_MESSAGE(
      check_close_enough(a, b, tol),
      "a=" << a << ", and b=" << b << " are not within tolerance=" << tol);
  BOOST_CHECK_MESSAGE(
      not check_close_enough(a, c, tol),
      "a=" << a << ", and c=" << c << " are within tolerance=" << tol);
}

/**
 * @test check_close_enough functionality for vector of float type
 */
BOOST_AUTO_TEST_CASE(check_close_enough_float_vector) {
  using namespace jb::testing;
  using value_type = float;
  using test_type = std::vector<value_type>;
  int tol = 3;
  int nsamples = 20;
  value_type num_a = 10.00;
  test_type a(nsamples, num_a);
  value_type num_b = num_a + std::numeric_limits<value_type>::epsilon();
  test_type b(nsamples, num_b);
  BOOST_CHECK_MESSAGE(
      check_collection_close_enough(a, b, tol),
      "a and b are not within tolerance=" << tol);
}

/**
 * @test check_close_enough functionality for vector of float type
 */
BOOST_AUTO_TEST_CASE(
    check_close_enough_float_vector_failure,
    *boost::unit_test::expected_failures(JB_TESTING_MAX_DIFFERENCES_PRINTED)) {
  using namespace jb::testing;
  using value_type = float;
  using test_type = std::vector<value_type>;
  int tol = 3;
  int nsamples = 20;
  value_type num_a = 10.00;
  test_type a(nsamples, num_a);
  value_type num_b =
      num_a +
      (value_type)(10 * tol) * std::numeric_limits<value_type>::epsilon();
  test_type b(nsamples, num_b);
  BOOST_CHECK_MESSAGE(
      not check_collection_close_enough(a, b, tol),
      "a and c are within tolerance=" << tol);
}

/**
 * @test check_close_enough functionality for multi_array of 3 dimension
 * complex double type
 */
BOOST_AUTO_TEST_CASE(
    check_close_enough_complex_double_multi_array,
    *boost::unit_test::expected_failures(JB_TESTING_MAX_DIFFERENCES_PRINTED)) {
  using namespace jb::testing;
  using value_type = double;
  using complex_type = std::complex<value_type>;
  using test_type = boost::multi_array<complex_type, 3>;
  int tol = 3;
  int S = 20;
  int V = 4;
  int nsamples = 2000;
  complex_type num_a = {10.00, 5.00};
  complex_type com_eps = {std::numeric_limits<value_type>::epsilon(),
                          std::numeric_limits<value_type>::epsilon()};
  complex_type num_b = num_a + com_eps;
  complex_type num_c = num_a + 10.0 * tol * com_eps;

  test_type a(boost::extents[S][V][nsamples]);
  test_type b(boost::extents[S][V][nsamples]);
  test_type c(boost::extents[S][V][nsamples]);

  for (int i = 0; i != S; ++i) {
    for (int j = 0; j != V; ++j) {
      for (int k = 0; k != nsamples; ++k) {
        a[i][j][k] = num_a;
        b[i][j][k] = num_b;
        c[i][j][k] = num_c;
      }
    }
  }

  BOOST_CHECK_MESSAGE(
      check_collection_close_enough(a, b, tol),
      "a and b are not within tolerance=" << tol);
  BOOST_CHECK_MESSAGE(
      not check_collection_close_enough(a, c, tol),
      "a and c are within tolerance=" << tol);
}
