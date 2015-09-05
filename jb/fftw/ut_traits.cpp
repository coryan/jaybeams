#include <jb/fftw/traits.hpp>
#include <jb/testing/check_array_close_enough.hpp>

#include <boost/test/unit_test.hpp>
#include <algorithm>

namespace {

template<typename precision_t>
void test_fftw_traits() {
  int nsamples = 32768;
  int tol = nsamples;
  typedef jb::fftw::traits<precision_t> tested;
  typedef typename tested::fftw_complex_type fftw_complex_type;

  fftw_complex_type* in = static_cast<fftw_complex_type*>(
      tested::allocate(nsamples * sizeof(fftw_complex_type)));
  fftw_complex_type* tmp = static_cast<fftw_complex_type*>(
      tested::allocate(nsamples * sizeof(fftw_complex_type)));
  fftw_complex_type* out = static_cast<fftw_complex_type*>(
      tested::allocate(nsamples * sizeof(fftw_complex_type)));

  std::size_t h = nsamples / 2;
  for (std::size_t i = 0; i != h; ++i) {
    in[i][0] = i - h/4.0;
    in[i][1] = 0;
    in[i + h][0] = h/4.0 - i;
    in[i + h][1] = 0;
  }

  typedef typename tested::fftw_plan_type plan_type;
  plan_type dir = tested::create_forward_plan(nsamples, in, tmp);
  plan_type inv = tested::create_backward_plan(nsamples, tmp, out);

  tested::execute_plan(dir, in, tmp);
  tested::execute_plan(inv, tmp, out);
  for (std::size_t i = 0; i != nsamples; ++i) {
    out[i][0] /= nsamples;
    out[i][1] /= nsamples;
  }
  jb::testing::check_array_close_enough(nsamples, out, in, tol);

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
  test_fftw_traits<long double>();
}
