#include <jb/fftw/traits.hpp>
#include <jb/testing/check_close_enough.hpp>
#include <valgrind/valgrind.h>

#include <boost/test/unit_test.hpp>
#include <algorithm>

namespace {

template <typename precision_t>
void test_fftw_traits() {
  int nsamples = 32768;
  int tol = nsamples;
  using tested = jb::fftw::traits<precision_t>;
  using fftw_complex_type = typename tested::fftw_complex_type;

  fftw_complex_type* in = static_cast<fftw_complex_type*>(
      tested::allocate(nsamples * sizeof(fftw_complex_type)));
  fftw_complex_type* tmp = static_cast<fftw_complex_type*>(
      tested::allocate(nsamples * sizeof(fftw_complex_type)));
  fftw_complex_type* out = static_cast<fftw_complex_type*>(
      tested::allocate(nsamples * sizeof(fftw_complex_type)));

  std::size_t h = nsamples / 2;
  for (std::size_t i = 0; i != h; ++i) {
    in[i][0] = i - h / 4.0;
    in[i][1] = 0;
    in[i + h][0] = h / 4.0 - i;
    in[i + h][1] = 0;
  }

  using plan_type = typename tested::fftw_plan_type;
  plan_type dir = tested::create_forward_plan(
      nsamples, in, tmp, FFTW_ESTIMATE | FFTW_UNALIGNED | FFTW_PRESERVE_INPUT);
  plan_type inv = tested::create_backward_plan(
      nsamples, tmp, out, FFTW_ESTIMATE | FFTW_UNALIGNED | FFTW_PRESERVE_INPUT);

  tested::execute_plan(dir, in, tmp);
  tested::execute_plan(inv, tmp, out);
  for (std::size_t i = 0; i != std::size_t(nsamples); ++i) {
    out[i][0] /= nsamples;
    out[i][1] /= nsamples;
  }
  bool res = jb::testing::check_collection_close_enough(nsamples, out, in, tol);
  BOOST_CHECK_MESSAGE(res, "collections are not within tolerance=" << tol);

  tested::destroy_plan(inv);
  tested::destroy_plan(dir);
  tested::release(out);
  tested::release(tmp);
  tested::release(in);
}

} // anonymous namespace

/**
 * @test Verify that we can compile jb::fftw::traits<double>
 */
BOOST_AUTO_TEST_CASE(fftw_traits_double) {
  test_fftw_traits<double>();
}

/**
 * @test Verify that we can compile jb::fftw::traits<float>
 */
BOOST_AUTO_TEST_CASE(fftw_traits_float) {
  test_fftw_traits<float>();
}

/**
 * @test Verify that we can compile jb::fftw::traits<long double>
 */
BOOST_AUTO_TEST_CASE(fftw_traits_long_double) {
  if (RUNNING_ON_VALGRIND > 0) {
    BOOST_TEST_MESSAGE("long double not supported by valgrind, skip test");
    return;
  }
  test_fftw_traits<long double>();
}
