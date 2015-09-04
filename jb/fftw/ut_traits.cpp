#include <jb/fftw/traits.hpp>
#include <jb/testing/check_equal_vector.hpp>

#include <boost/test/unit_test.hpp>
#include <algorithm>

/**
 * @test Verify that we can compile jb::fftw::traints<double>
 */
BOOST_AUTO_TEST_CASE(fftw_traits_double) {
  int nsamples = 32768;
  int tol = nsamples;
  typedef jb::fftw::traits<double> tested;
  typedef tested::fftw_complex_type fftw_complex_type;

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
  
  tested::fftw_plan_type dir = tested::create_forward_plan(nsamples, in, tmp);
  tested::fftw_plan_type inv = tested::create_backward_plan(nsamples, tmp, out);

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
