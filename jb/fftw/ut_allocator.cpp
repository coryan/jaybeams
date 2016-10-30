#include <jb/fftw/allocator.hpp>
#include <jb/fftw/plan.hpp>
#include <jb/testing/check_vector_close_enough.hpp>

#include <boost/test/unit_test.hpp>
#include <vector>

namespace {

template <typename T>
using aligned_vector = std::vector<T, jb::fftw::allocator<T>>;

template <typename precision_t> void test_plan_real2complex() {
  int nsamples = 1 << 15;
  int tol = nsamples;

  typedef jb::fftw::plan<precision_t> tested;
  typedef typename tested::precision_type precision_type;
  typedef std::complex<precision_type> complex;

  aligned_vector<precision_t> in(nsamples);
  aligned_vector<complex> tmp(nsamples);
  aligned_vector<precision_t> out(nsamples);

  std::size_t h = nsamples / 2;
  for (std::size_t i = 0; i != h; ++i) {
    in[i] = i - h / 4.0;
    in[i + h] = h / 4.0 - i;
  }

  tested dir = tested::create_forward(in, tmp);
  tested inv = tested::create_backward(tmp, out);

  dir.execute(in, tmp);
  inv.execute(tmp, out);
  for (std::size_t i = 0; i != std::size_t(nsamples); ++i) {
    out[i] /= nsamples;
  }
  jb::testing::check_vector_close_enough(out, in, tol);
}

} // anonymous namespace

/**
 * @test Verify that we can use jb::fftw::allocator<>
 */
BOOST_AUTO_TEST_CASE(fftw_allocator_double) {
  test_plan_real2complex<double>();
}

/**
 * @test Verify that we can use jb::fftw::allocator<>
 */
BOOST_AUTO_TEST_CASE(fftw_allocator_float) {
  test_plan_real2complex<float>();
}

/**
 * @test Verify that we can use jb::fftw::allocator<>
 */
BOOST_AUTO_TEST_CASE(fftw_allocator_long_double) {
  test_plan_real2complex<long double>();
}
