#include <jb/fftw/aligned_multi_array.hpp>
#include <jb/fftw/cast.hpp>

#include <boost/test/unit_test.hpp>

/**
 * Helper functions to test fftw_cast for boost::multi_array<>
 */
namespace {

/// Test cast functions for a given array type
template <typename array_type, typename expected>
void check_cast_array_3d() {
  BOOST_TEST_MESSAGE("Testing for " << __FUNCTION__);
  std::size_t const F = 2;
  std::size_t const S = 128;
  std::size_t const N = 1 << 15;

  array_type v(boost::extents[F][S][N]);
  auto p = jb::fftw::fftw_cast(v);
  bool actual = std::is_same<decltype(p), expected>::value;
  BOOST_CHECK_EQUAL(actual, true);
}

} // anonymous namespace

using jb::fftw::aligned_multi_array;
using boost::multi_array;

/**
 * @test Verify that fftw_cast functions work for boost::multi_array<float>
 */
BOOST_AUTO_TEST_CASE(fftw_cast_array_3d_float) {
  check_cast_array_3d<multi_array<float, 3>, float*>();
  check_cast_array_3d<multi_array<float, 3> const, float const*>();

  check_cast_array_3d<aligned_multi_array<float, 3>, float*>();
  check_cast_array_3d<aligned_multi_array<float, 3> const, float const*>();
}

/**
 * @test Verify that fftw_cast functions work for boost::multi_array<double>
 */
BOOST_AUTO_TEST_CASE(fftw_cast_array_3d_double) {
  check_cast_array_3d<boost::multi_array<double, 3>, double*>();
  check_cast_array_3d<boost::multi_array<double, 3> const, double const*>();

  check_cast_array_3d<jb::fftw::aligned_multi_array<double, 3>, double*>();
  check_cast_array_3d<
      jb::fftw::aligned_multi_array<double, 3> const, double const*>();
}

/**
 * @test Verify that fftw_cast functions work for
 * boost::multi_array<long double>
 */
BOOST_AUTO_TEST_CASE(fftw_cast_array_3d_long_double) {
  check_cast_array_3d<boost::multi_array<long double, 3>, long double*>();
  check_cast_array_3d<
      boost::multi_array<long double, 3> const, long double const*>();

  check_cast_array_3d<
      jb::fftw::aligned_multi_array<long double, 3>, long double*>();
  check_cast_array_3d<
      jb::fftw::aligned_multi_array<long double, 3> const,
      long double const*>();
}

/**
 * @test Verify that fftw_cast functions work for
 * boost::multi_array<std::complex<float>>
 */
BOOST_AUTO_TEST_CASE(fftw_cast_array_3d_complex_float) {
  check_cast_array_3d<
      boost::multi_array<std::complex<float>, 3>, fftwf_complex*>();
  check_cast_array_3d<
      boost::multi_array<std::complex<float>, 3> const, fftwf_complex const*>();

  check_cast_array_3d<
      jb::fftw::aligned_multi_array<std::complex<float>, 3>, fftwf_complex*>();
  check_cast_array_3d<
      jb::fftw::aligned_multi_array<std::complex<float>, 3> const,
      fftwf_complex const*>();
}

/**
 * @test Verify that fftw_cast functions work for
 * boost::multi_array<std::complex<double>>
 */
BOOST_AUTO_TEST_CASE(fftw_cast_array_3d_complex_double) {
  check_cast_array_3d<
      boost::multi_array<std::complex<double>, 3>, fftw_complex*>();
  check_cast_array_3d<
      boost::multi_array<std::complex<double>, 3> const, fftw_complex const*>();

  check_cast_array_3d<
      jb::fftw::aligned_multi_array<std::complex<double>, 3>, fftw_complex*>();
  check_cast_array_3d<
      jb::fftw::aligned_multi_array<std::complex<double>, 3> const,
      fftw_complex const*>();
}

/**
 * @test Verify that fftw_cast functions work for
 * boost::multi_array<std::complex<long double>>
 */
BOOST_AUTO_TEST_CASE(fftw_cast_array_3d_complex_long_double) {
  check_cast_array_3d<
      boost::multi_array<std::complex<long double>, 3>, fftwl_complex*>();
  check_cast_array_3d<
      boost::multi_array<std::complex<long double>, 3> const,
      fftwl_complex const*>();

  check_cast_array_3d<
      jb::fftw::aligned_multi_array<std::complex<long double>, 3>,
      fftwl_complex*>();
  check_cast_array_3d<
      jb::fftw::aligned_multi_array<std::complex<long double>, 3> const,
      fftwl_complex const*>();
}
