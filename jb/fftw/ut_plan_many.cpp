#include <jb/fftw/plan.hpp>
#include <jb/testing/check_multi_array_close_enough.hpp>

#include <boost/multi_array.hpp>
#include <boost/test/unit_test.hpp>
#include <algorithm>

/**
 * Helper functions to test jb::fftw::plan in the many timeseries cases
 */
namespace {

/// Verify that plans work for a batch of complex vectors into a batch
/// of complex vectors
template <typename precision_t>
void test_plan_complex2complex() {
  // Define some constants used to size the test, the size does not
  // matter as much as the fact that we are testing multiple
  // dimensions.
  int const F = 2;
  int const S = 128;
  int const nsamples = 1 << 15;
  // The tolerance (define as a fraction of 1.0) depends on the size
  // of the test because the error in a FFT depends on the size of the
  // vector:
  //   https://en.wikipedia.org/wiki/Fast_Fourier_transform#Accuracy
  // we use a very rough approximation to the error here ...
  int const tol = nsamples;

  // ... this case is all about testing the FFT transforms between
  // multiple vectors of std::complex<> values ...
  typedef std::complex<precision_t> complex;

  // ... we define an input array, an intermediate array to contain
  // the FFT, and an output array that should (if things work Okay)
  // contain the same contents as the input after a FFT and inverse
  // FFT transformation ...
  boost::multi_array<complex, 3> in(boost::extents[F][S][nsamples]);
  boost::multi_array<complex, 3> tmp(boost::extents[F][S][nsamples]);
  boost::multi_array<complex, 3> out(boost::extents[F][S][nsamples]);

  // ... initialize the input with a series of pretty straight forward
  // triangular functions ...
  for (auto subarray : in) {
    for (auto vector : subarray) {
      std::size_t h = nsamples / 2;
      for (std::size_t i = 0; i != h; ++i) {
        vector[i] = complex(i - h / 4.0, 0);
        vector[i + h] = complex(h / 4.0 - i, 0);
      }
    }
  }

  // ... create a plan to apply the DFT (direct, not inverse) from the
  // in array to the tmp array ...
  auto dir = jb::fftw::create_forward_plan(in, tmp);
  // ... create a plan to apply the inverse DFT from the tmp array to
  // the out array ...
  auto inv = jb::fftw::create_backward_plan(tmp, out);

  // ... execute the direct transform ...
  dir.execute(in, tmp);
  // ... execute the inverse transform ...
  inv.execute(tmp, out);

  // ... a well known fact of the FFT implementation in FFTW: it does
  // not rescale the inverse transform by 1/N, so we must do that
  // manually, ugh ...
  for (auto subarray : out) {
    for (auto vector : subarray) {
      for (auto& element : vector) {
        element /= nsamples;
      }
    }
  }
  // ... compare the input to the output, they should be nearly
  // identical ...
  jb::testing::check_multi_array_close_enough(out, in, tol);
}

/// A generic test parametric on the precision of the floating point
template <typename precision_t>
void test_plan_errors() {
  // Define some constants used to size the test, the size does not
  // matter as much as the fact that we are testing multiple
  // dimensions.
  int const F = 2;
  int const S = 128;
  int const nsamples = 1 << 15;

  // ... this case is all about testing the FFT transforms between
  // multiple vectors of std::complex<> values ...
  typedef std::complex<precision_t> complex;

  boost::multi_array<complex, 3> a0(boost::extents[F][S][nsamples]);
  boost::multi_array<complex, 3> a1(boost::extents[F][S / 2][nsamples]);
  boost::multi_array<complex, 3> a2(boost::extents[F][S][nsamples / 2]);
  boost::multi_array<complex, 3> a3(boost::extents[F][S][0]);
  boost::multi_array<complex, 3> a4(boost::extents[F][S][nsamples]);

  BOOST_CHECK_THROW(jb::fftw::create_forward_plan(a0, a1), std::exception);
  BOOST_CHECK_THROW(jb::fftw::create_forward_plan(a0, a2), std::exception);
  BOOST_CHECK_THROW(jb::fftw::create_forward_plan(a0, a3), std::exception);
  BOOST_CHECK_THROW(jb::fftw::create_backward_plan(a1, a0), std::exception);
  BOOST_CHECK_THROW(jb::fftw::create_backward_plan(a2, a0), std::exception);
  BOOST_CHECK_THROW(jb::fftw::create_backward_plan(a3, a0), std::exception);

  boost::multi_array<precision_t, 3> b1(boost::extents[F][S / 2][nsamples]);
  boost::multi_array<precision_t, 3> b2(boost::extents[F][S][nsamples / 2]);
  boost::multi_array<precision_t, 3> b3(boost::extents[F][S][0]);
  BOOST_CHECK_THROW(jb::fftw::create_forward_plan(b1, a0), std::exception);
  BOOST_CHECK_THROW(jb::fftw::create_forward_plan(b2, a0), std::exception);
  BOOST_CHECK_THROW(jb::fftw::create_forward_plan(b3, a0), std::exception);
  BOOST_CHECK_THROW(jb::fftw::create_backward_plan(a0, b1), std::exception);
  BOOST_CHECK_THROW(jb::fftw::create_backward_plan(a0, b2), std::exception);
  BOOST_CHECK_THROW(jb::fftw::create_backward_plan(a0, b3), std::exception);
  
  auto dir = jb::fftw::create_forward_plan(a0, a4);
  BOOST_CHECK_THROW(dir.execute(a0, a1), std::exception);
}

} // anonymous namespace

/**
 * @test Verify that we can create and execute plans that convert
 * arrays of std::complex<double> to arrays of std::complex<double>.
 */
BOOST_AUTO_TEST_CASE(fftw_plan_many_complex_double) {
  test_plan_complex2complex<double>();
}

/**
 * @test Verify that we can create and execute plans that convert
 * arrays of std::complex<double> to arrays of std::complex<float>.
 */
BOOST_AUTO_TEST_CASE(fftw_plan_many_complex_float) {
  test_plan_complex2complex<float>();
}

/**
 * @test Verify that we can create and execute plans that convert
 * arrays of std::complex<double> to arrays of std::complex<long double>.
 */
BOOST_AUTO_TEST_CASE(fftw_plan_many_complex_long_double) {
  test_plan_complex2complex<long double>();
}

/**
 * @test Verify that jb::fftw::create_foward_plan_1d() and
 * jb::fftw::create_backward_plan_1d() detect errors correctly for
 * double.
 */
BOOST_AUTO_TEST_CASE(fftw_plan_many_error_double) {
  test_plan_errors<double>();
}

/**
 * @test Verify that jb::fftw::create_foward_plan_1d() and
 * jb::fftw::create_backward_plan_1d() detect errors correctly for
 * float.
 */
BOOST_AUTO_TEST_CASE(fftw_plan_many_error_float) {
  test_plan_errors<float>();
}

/**
 * @test Verify that jb::fftw::create_foward_plan_1d() and
 * jb::fftw::create_backward_plan_1d() detect errors correctly for
 * long double.
 */
BOOST_AUTO_TEST_CASE(fftw_plan_many_error_long_double) {
  test_plan_errors<long double>();
}
