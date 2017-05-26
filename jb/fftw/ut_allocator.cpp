#include <jb/fftw/allocator.hpp>
#include <jb/fftw/plan.hpp>
#include <jb/testing/check_close_enough.hpp>
#include <valgrind/valgrind.h>

#include <boost/test/unit_test.hpp>
#include <vector>

namespace {

template <typename T>
using aligned_vector = std::vector<T, jb::fftw::allocator<T>>;

template <typename precision_t>
void test_plan_real2complex() {
  int nsamples = 1 << 15;
  int tol = nsamples;

  typedef std::complex<precision_t> complex;

  aligned_vector<precision_t> in(nsamples);
  aligned_vector<complex> tmp(nsamples);
  aligned_vector<precision_t> out(nsamples);

  std::size_t h = nsamples / 2;
  for (std::size_t i = 0; i != h; ++i) {
    in[i] = i - h / 4.0;
    in[i + h] = h / 4.0 - i;
  }

  auto dir = jb::fftw::create_forward_plan(in, tmp);
  auto inv = jb::fftw::create_backward_plan(tmp, out);

  dir.execute(in, tmp);
  inv.execute(tmp, out);
  for (std::size_t i = 0; i != std::size_t(nsamples); ++i) {
    out[i] /= nsamples;
  }
  bool res = jb::testing::check_collection_close_enough(out, in, tol);
  BOOST_CHECK_MESSAGE(res, "collections are not within tolerance=" << tol);
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
  if (RUNNING_ON_VALGRIND > 0) {
    BOOST_TEST_MESSAGE("long double not supported by valgrind, skip test");
    return;
  }
  test_plan_real2complex<long double>();
}
