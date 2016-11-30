#include <jb/fftw/aligned_multi_array.hpp>

#include <boost/test/unit_test.hpp>

/**
 * Helper functions to test aligned arrays
 */
namespace {

std::size_t check_dynamic_size(int F, int S, int N) {
  jb::fftw::aligned_multi_array<float, 4> A(boost::extents[F][S][4][N]);
  return A.num_elements();
}

} // anonymous namespace

BOOST_AUTO_TEST_CASE(multiarray_basic) {
  int const F = 2;
  int const S = 128;
  int const N = 16384;

  auto actual = check_dynamic_size(F, S, N);
  BOOST_CHECK_EQUAL(actual, std::size_t(F * S * 4 * N));

  jb::fftw::aligned_multi_array<int, 4> A(boost::extents[F][S][4][N]);
  boost::multi_array_ref<int, 2> R(A.data(), boost::extents[F * S * 4][N]);

  BOOST_CHECK_EQUAL(R.size(), std::size_t(F * S * 4));
  BOOST_CHECK_EQUAL(R[0].size(), std::size_t(N));
  BOOST_CHECK_EQUAL(R.num_elements(), std::size_t(F * S * 4 * N));

  A[0][0][0][0] = 100;
  BOOST_CHECK_EQUAL(A[0][0][0][0], 100);

  R[0][0] = 200;
  BOOST_CHECK_EQUAL(A[0][0][0][0], 200);

  boost::multi_array_ref<int, 2> R2(R);
  R2[0][0] = 300;
  BOOST_CHECK_EQUAL(A[0][0][0][0], 300);

  for (auto const& v : R) {
    BOOST_CHECK_EQUAL(v.size(), N);
  }
}
