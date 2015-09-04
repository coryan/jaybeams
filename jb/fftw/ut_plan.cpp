#include <jb/fftw/plan.hpp>
#include <jb/testing/check_equal_vector.hpp>

#include <boost/test/unit_test.hpp>
#include <algorithm>

/**
 * @test Verify that we can create fftw_plan.
 */
BOOST_AUTO_TEST_CASE(fftw_plan_complex_double) {
  int nsamples = 1<<15;
  int tol = nsamples;

  typedef jb::fftw::plan<double> tested;
  typedef tested::fftw_complex_type fftw_complex_type;

  fftw_complex_type* in = static_cast<fftw_complex_type*>(
      tested::traits::allocate(nsamples * sizeof(fftw_complex_type)));
  fftw_complex_type* tmp = static_cast<fftw_complex_type*>(
      tested::traits::allocate(nsamples * sizeof(fftw_complex_type)));
  fftw_complex_type* out = static_cast<fftw_complex_type*>(
      tested::traits::allocate(nsamples * sizeof(fftw_complex_type)));

  std::size_t h = nsamples / 2;
  for (std::size_t i = 0; i != h; ++i) {
    in[i][0] = i - h/4.0;
    in[i][1] = 0;
    in[i + h][0] = h/4.0 - i;
    in[i + h][1] = 0;
  }
  
  tested dir = tested::create_forward(nsamples, in, tmp);
  tested inv = tested::create_backward(nsamples, tmp, out);

  dir.execute(in, tmp);
  inv.execute(tmp, out);
  for (std::size_t i = 0; i != nsamples; ++i) {
    out[i][0] /= nsamples;
    out[i][1] /= nsamples;
  }
  jb::testing::check_array_close_enough(nsamples, out, in, tol);

  tested::traits::release(out);
  tested::traits::release(tmp);
  tested::traits::release(in);
}
