#include <jb/fftw/aligned_vector.hpp>
#include <jb/fftw/cast.hpp>

#include <boost/test/unit_test.hpp>

/**
 * Helper functions to test fftw_cast
 */
namespace {

/// Test cast functions for a given vector type
template <typename vector_type, typename expected>
void check_cast_vector() {
  BOOST_TEST_MESSAGE("Testing in " << __FUNCTION__);
  std::size_t const N = 1 << 15;

  vector_type v(N);
  auto p = jb::fftw::fftw_cast(v);
  bool actual = std::is_same<decltype(p), expected>::value;
  BOOST_CHECK_EQUAL(actual, true);
}

} // anonymous namespace

/**
 * @test Verify that fftw_cast functions work for std::vector<float>
 */
BOOST_AUTO_TEST_CASE(fftw_cast_vector_float) {
  check_cast_vector<std::vector<float>, float*>();
  check_cast_vector<std::vector<float> const, float const*>();

  check_cast_vector<jb::fftw::aligned_vector<float>, float*>();
  check_cast_vector<jb::fftw::aligned_vector<float> const, float const*>();
}

/**
 * @test Verify that fftw_cast functions work for std::vector<double>
 */
BOOST_AUTO_TEST_CASE(fftw_cast_vector_double) {
  check_cast_vector<std::vector<double>, double*>();
  check_cast_vector<std::vector<double> const, double const*>();

  check_cast_vector<jb::fftw::aligned_vector<double>, double*>();
  check_cast_vector<jb::fftw::aligned_vector<double> const, double const*>();
}

/**
 * @test Verify that fftw_cast functions work for std::vector<long double>
 */
BOOST_AUTO_TEST_CASE(fftw_cast_vector_long_double) {
  check_cast_vector<std::vector<long double>, long double*>();
  check_cast_vector<std::vector<long double> const, long double const*>();

  check_cast_vector<jb::fftw::aligned_vector<long double>, long double*>();
  check_cast_vector<
      jb::fftw::aligned_vector<long double> const, long double const*>();
}

/**
 * @test Verify that fftw_cast functions work for
 * std::vector<std::complex<float>>
 */
BOOST_AUTO_TEST_CASE(fftw_cast_vector_complex_float) {
  check_cast_vector<std::vector<std::complex<float>>, fftwf_complex*>();
  check_cast_vector<
      std::vector<std::complex<float>> const, fftwf_complex const*>();

  check_cast_vector<
      jb::fftw::aligned_vector<std::complex<float>>, fftwf_complex*>();
  check_cast_vector<
      jb::fftw::aligned_vector<std::complex<float>> const,
      fftwf_complex const*>();
}

/**
 * @test Verify that fftw_cast functions work for
 * std::vector<std::complex<double>>
 */
BOOST_AUTO_TEST_CASE(fftw_cast_vector_complex_double) {
  check_cast_vector<std::vector<std::complex<double>>, fftw_complex*>();
  check_cast_vector<
      std::vector<std::complex<double>> const, fftw_complex const*>();

  check_cast_vector<
      jb::fftw::aligned_vector<std::complex<double>>, fftw_complex*>();
  check_cast_vector<
      jb::fftw::aligned_vector<std::complex<double>> const,
      fftw_complex const*>();
}

/**
 * @test Verify that fftw_cast functions work for
 * std::vector<std::complex<long double>>
 */
BOOST_AUTO_TEST_CASE(fftw_cast_vector_complex_long_double) {
  check_cast_vector<std::vector<std::complex<long double>>, fftwl_complex*>();
  check_cast_vector<
      std::vector<std::complex<long double>> const, fftwl_complex const*>();

  check_cast_vector<
      jb::fftw::aligned_vector<std::complex<long double>>, fftwl_complex*>();
  check_cast_vector<
      jb::fftw::aligned_vector<std::complex<long double>> const,
      fftwl_complex const*>();
}
