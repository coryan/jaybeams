#include <jb/fftw/plan.hpp>
#include <jb/testing/check_close_enough.hpp>
#include <valgrind/valgrind.h>

#include <boost/test/unit_test.hpp>
#include <algorithm>

/**
 * Helper functions to test jb::fftw::plan
 */
namespace {

template <typename precision_t>
void test_plan_complex2complex() {
  int nsamples = 1 << 15;
  int tol = nsamples;

  typedef std::complex<precision_t> complex;

  std::vector<complex> in(nsamples);
  std::vector<complex> tmp(nsamples);
  std::vector<complex> out(nsamples);

  std::size_t h = nsamples / 2;
  for (std::size_t i = 0; i != h; ++i) {
    in[i] = complex(i - h / 4.0, 0);
    in[i + h] = complex(h / 4.0 - i, 0);
  }

  auto dir = jb::fftw::create_forward_plan(in, tmp);
  auto inv = jb::fftw::create_backward_plan(tmp, out);

  dir.execute(in, tmp);
  inv.execute(tmp, out);
  for (std::size_t i = 0; i != std::size_t(nsamples); ++i) {
    out[i] /= nsamples;
  }
  jb::testing::check_collection_close_enough(out, in, tol);
}

template <typename precision_t>
void test_plan_real2complex() {
  int nsamples = 1 << 15;
  int tol = nsamples;

  typedef std::complex<precision_t> complex;

  std::vector<precision_t> in(nsamples);
  std::vector<complex> tmp(nsamples);
  std::vector<precision_t> out(nsamples);

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
  jb::testing::check_collection_close_enough(out, in, tol);
}

template <typename precision_t>
void test_plan_errors() {
  int nsamples = 1 << 15;

  typedef std::complex<precision_t> complex;

  std::vector<complex> in(nsamples);
  std::vector<complex> tmp(nsamples);
  std::vector<complex> err(nsamples / 2);

  BOOST_CHECK_THROW(jb::fftw::create_forward_plan(in, err), std::exception);
  BOOST_CHECK_THROW(jb::fftw::create_backward_plan(in, err), std::exception);

  auto dir = jb::fftw::create_forward_plan(in, tmp);
  BOOST_CHECK_THROW(dir.execute(in, err), std::exception);
}

} // anonymous namespace

/**
 * @test Verify that we can create and operate a jb::fftw::plan<double>
 */
BOOST_AUTO_TEST_CASE(fftw_plan_complex_double) {
  test_plan_complex2complex<double>();
}

/**
 * @test Verify that we can create and operate a jb::fftw::plan<double>
 */
BOOST_AUTO_TEST_CASE(fftw_plan_double) {
  test_plan_real2complex<double>();
}

/**
 * @test Verify jb::fftw::plan<double> detects obvious errors
 */
BOOST_AUTO_TEST_CASE(fftw_plan_error_double) {
  test_plan_errors<double>();
}

/**
 * @test Verify that we can create and operate a jb::fftw::plan<float>
 */
BOOST_AUTO_TEST_CASE(fftw_plan_complex_float) {
  test_plan_complex2complex<float>();
}

/**
 * @test Verify that we can create and operate a jb::fftw::plan<float>
 */
BOOST_AUTO_TEST_CASE(fftw_plan_float) {
  test_plan_real2complex<float>();
}

/**
 * @test Verify jb::fftw::plan<float> detects obvious errors
 */
BOOST_AUTO_TEST_CASE(fftw_plan_error_float) {
  test_plan_errors<float>();
}

/**
 * @test Verify that we can create and operate a jb::fftw::plan<long double>
 */
BOOST_AUTO_TEST_CASE(fftw_plan_complex_long_double) {
  if (RUNNING_ON_VALGRIND > 0) {
    BOOST_TEST_MESSAGE("long double not supported by valgrind, skip test");
    return;
  }
  test_plan_complex2complex<long double>();
}

/**
 * @test Verify that we can create and operate a jb::fftw::plan<long double>
 */
BOOST_AUTO_TEST_CASE(fftw_plan_long_double) {
  if (RUNNING_ON_VALGRIND > 0) {
    BOOST_TEST_MESSAGE("long double not supported by valgrind, skip test");
    return;
  }
  test_plan_real2complex<long double>();
}

/**
 * @test Verify jb::fftw::plan<long double> detects obvious errors
 */
BOOST_AUTO_TEST_CASE(fftw_plan_error_long_double) {
  if (RUNNING_ON_VALGRIND > 0) {
    BOOST_TEST_MESSAGE("long double not supported by valgrind, skip test");
    return;
  }
  test_plan_errors<long double>();
}
