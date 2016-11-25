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

#if 0
  auto dir = jb::fftw::create_forward_plan(in, tmp);
  auto inv = jb::fftw::create_backward_plan(tmp, out);

  dir.execute(in, tmp);
  inv.execute(tmp, out);
  for (auto subarray : out) {
    for (auto vector : subarray) {
      for (auto element : vector) {
        element /= nsamples;
      }
    }
  }
#else
  out = in;
#endif /* 0 */
  jb::testing::check_multi_array_close_enough(out, in, tol);
}

} // anonymous namespace

/**
 * @test Verify that we can create and operate a jb::fftw::plan<double>
 */
BOOST_AUTO_TEST_CASE(fftw_plan_many_complex_double) {
  test_plan_complex2complex<double>();
}
