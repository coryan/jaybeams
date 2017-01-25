#include <jb/fftw/tde_result.hpp>
#include <jb/fftw/time_delay_estimator_many.hpp>
#include <jb/testing/check_tde_result_close_enough.hpp>
#include <jb/testing/create_square_timeseries.hpp>
#include <jb/testing/create_triangle_timeseries.hpp>
#include <jb/testing/delay_timeseries.hpp>

#include <boost/test/unit_test.hpp>
#include <chrono>

/**
 * @test Verify that we can create and use a simple time delay estimator.
 * dimension 3
 */
BOOST_AUTO_TEST_CASE(fftw_time_delay_estimator_many_3_dim_tde) {
  int const nsamples = 1 << 15;
  int const S = 20;
  int const V = 4;
  int const delay = 2500;
  int const tol = 2;

  using array_type = jb::fftw::aligned_multi_array<float, 3>;
  using tested_type = jb::fftw::time_delay_estimator_many<array_type>;
  using confidence_type = typename tested_type::confidence_type;
  using argmax_type = typename tested_type::argmax_type;

  array_type a(boost::extents[S][V][nsamples]);
  array_type b(boost::extents[S][V][nsamples]);
  confidence_type confidence(a);
  // expected confidence result to compare within tolerance tol
  confidence_type expected_confidence(a);

  argmax_type argmax(a);
  // expected argmax result to compare within tolerance tol
  argmax_type expected_argmax(a);

  using sum2_type = jb::fftw::tde_result<array_type, float>;
  // sum of square value of timeseries
  sum2_type sum2(b);

  // construct the tested FTE
  tested_type tested(a, b);

  // fill the timeseries with triangle or square
  jb::testing::create_triangle_timeseries(nsamples, b);

  // a = delay shift timeseries b
  int count = 0;
  for (int i = 0; i != S; ++i) {
    for (int j = 0; j != V; ++j, ++count) {
      float acc_sum2 = 0;
      for (int k = 0; k != nsamples; ++k) {
        a[i][j][k] = b[i][j][(k + delay) % nsamples];
        acc_sum2 += a[i][j][k] * a[i][j][k];
      }
      sum2[count] = acc_sum2;
      expected_argmax[count] = static_cast<std::size_t>(delay);
      expected_confidence[count] = static_cast<float>(nsamples);
    }
  }

  // run the TDE...
  (void)tested.estimate_delay(a, b, sum2, confidence, argmax);
  // check argmax within delay +/- tol
  BOOST_CHECK_MESSAGE(
      jb::testing::tde_result_close_enough(argmax, expected_argmax, tol),
      "argmax is not within tolerance("
          << tol << ", argmax[0]=" << argmax[0]
          << ", expected_argmax[0]=" << expected_argmax[0]);
  // check confidence within nsamples +/- tol
  BOOST_CHECK_MESSAGE(
      jb::testing::tde_result_close_enough(
          confidence, expected_confidence, 10 * tol),
      "confidence is not within tolerance(" << 10 * tol << ")");
}

/**
 * @test Verify that we can create and use a simple time delay estimator.
 * dimension 2
 */
BOOST_AUTO_TEST_CASE(fftw_time_delay_estimator_many_2_dim_tde) {
  int const nsamples = 1 << 15;
  int const S = 20;
  int const delay = 2500;
  int const tol = 2;

  using array_type = jb::fftw::aligned_multi_array<float, 2>;
  using tested_type = jb::fftw::time_delay_estimator_many<array_type>;
  using confidence_type = typename tested_type::confidence_type;
  using argmax_type = typename tested_type::argmax_type;

  array_type a(boost::extents[S][nsamples]);
  array_type b(boost::extents[S][nsamples]);
  confidence_type confidence(a);
  // expected confidence result to compare within tolerance tol
  confidence_type expected_confidence(a);

  argmax_type argmax(a);
  // expected argmax result to compare within tolerance tol
  argmax_type expected_argmax(a);

  using sum2_type = jb::fftw::tde_result<array_type, float>;
  sum2_type sum2(b);

  // construct the tested FTE
  tested_type tested(a, b);

  // fill the timeseries with triangle or square
  jb::testing::create_triangle_timeseries(nsamples, b);

  // a = delay shift timeseries b
  int count = 0;
  for (int i = 0; i != S; ++i) {
    float acc_sum2 = 0;
    for (int k = 0; k != nsamples; ++k) {
      a[i][k] = b[i][(k + delay) % nsamples];
      acc_sum2 += a[i][k] * a[i][k];
    }
    sum2[count] = acc_sum2;
    expected_argmax[count] = static_cast<std::size_t>(delay);
    expected_confidence[count] = static_cast<float>(nsamples);
    count++;
  }

  // run TDE ....
  (void)tested.estimate_delay(a, b, sum2, confidence, argmax);
  // check argmax within delay +/- tol
  BOOST_CHECK_MESSAGE(
      jb::testing::tde_result_close_enough(argmax, expected_argmax, tol),
      "argmax is not within tolerance("
          << tol << ", argmax[0]=" << argmax[0]
          << ", expected_argmax[0]=" << expected_argmax[0]);
  // check confidence within nsamples +/- tol
  BOOST_CHECK_MESSAGE(
      jb::testing::tde_result_close_enough(
          confidence, expected_confidence, 10 * tol),
      "confidence is not within tolerance(" << 10 * tol << ")");
}

/**
 * @test Verify that we can create and use a simple time delay estimator.
 * dimension 1
 */
BOOST_AUTO_TEST_CASE(fftw_time_delay_estimator_many_1_dim_tde) {
  int const nsamples = 1 << 15;
  int const delay = 2500;
  int const tol = 2;

  using array_type = jb::fftw::aligned_multi_array<float, 1>;
  using tested_type = jb::fftw::time_delay_estimator_many<array_type>;
  using confidence_type = typename tested_type::confidence_type;
  using argmax_type = typename tested_type::argmax_type;

  array_type a(boost::extents[nsamples]);
  array_type b(boost::extents[nsamples]);
  confidence_type confidence(a);
  // expected confidence result to compare within tolerance tol
  confidence_type expected_confidence(a);

  argmax_type argmax(a);
  // expected argmax result to compare within tolerance tol
  argmax_type expected_argmax(a);

  using sum2_type = jb::fftw::tde_result<array_type, float>;
  sum2_type sum2(b);

  // construct the tested FTE
  tested_type tested(a, b);

  // fill the timeseries with triangle or square
  jb::testing::create_triangle_timeseries(nsamples, b);

  // a = delay shift timeseries b
  float acc_sum2 = 0;
  for (int k = 0; k != nsamples; ++k) {
    a[k] = b[(k + delay) % nsamples];
    acc_sum2 += a[k] * a[k];
  }
  sum2[0] = acc_sum2;
  expected_argmax[0] = static_cast<std::size_t>(delay);
  expected_confidence[0] = static_cast<float>(nsamples);

  (void)tested.estimate_delay(a, b, sum2, confidence, argmax);
  // check argmax within delay +/- tol
  BOOST_CHECK_MESSAGE(
      jb::testing::tde_result_close_enough(argmax, expected_argmax, tol),
      "argmax is not within tolerance("
          << tol << ", argmax[0]=" << argmax[0]
          << ", expected_argmax[0]=" << expected_argmax[0]);
  // check confidence within nsamples +/- tol
  BOOST_CHECK_MESSAGE(
      jb::testing::tde_result_close_enough(
          confidence, expected_confidence, 10 * tol),
      "confidence is not within tolerance(" << 10 * tol << ")");
}

/**
 * @test Verify that FTE handles a timeseries filled with 0
 * dimension 3
 */
BOOST_AUTO_TEST_CASE(fftw_time_delay_estimator_many_3_dim_tde_with_0) {
  int const nsamples = 1 << 15;
  int const S = 20;
  int const V = 4;
  int const tol = 2;

  using array_type = jb::fftw::aligned_multi_array<float, 3>;
  using tested_type = jb::fftw::time_delay_estimator_many<array_type>;
  using confidence_type = typename tested_type::confidence_type;
  using argmax_type = typename tested_type::argmax_type;

  array_type a(boost::extents[S][V][nsamples]);
  array_type b(boost::extents[S][V][nsamples]);
  confidence_type confidence(a);
  // expected confidence result to compare within tolerance tol
  confidence_type expected_confidence(a);

  argmax_type argmax(a);
  // expected argmax result to compare within tolerance tol
  argmax_type expected_argmax(a);

  using sum2_type = jb::fftw::tde_result<array_type, float>;
  // sum of square value of timeseries
  sum2_type sum2(b);

  // construct the tested FTE
  tested_type tested(a, b);

  // fill both timeseries with 0
  int count = 0;
  for (int i = 0; i != S; ++i) {
    for (int j = 0; j != V; ++j, ++count) {
      for (int k = 0; k != nsamples; ++k) {
        a[i][j][k] = float(0);
        b[i][j][k] = float(0);
      }
      sum2[count] = float(0);
      expected_argmax[count] = static_cast<std::size_t>(0);
      expected_confidence[count] = std::numeric_limits<float>::max();
    }
  }

  // run the TDE...
  (void)tested.estimate_delay(a, b, sum2, confidence, argmax);
  // check argmax within delay +/- tol
  BOOST_CHECK_MESSAGE(
      jb::testing::tde_result_close_enough(argmax, expected_argmax, tol),
      "argmax is not within tolerance("
          << tol << ", argmax[0]=" << argmax[0]
          << ", expected_argmax[0]=" << expected_argmax[0]);
  // check confidence within nsamples +/- tol
  BOOST_CHECK_MESSAGE(
      jb::testing::tde_result_close_enough(
          confidence, expected_confidence, 10 * tol),
      "confidence is not within tolerance(" << 10 * tol << ")");
}

/**
 * @test Verify TDE handles exact same timeseries, delay = 0
 * dimension 3
 */
BOOST_AUTO_TEST_CASE(fftw_time_delay_estimator_many_3_dim_tde_delay_0) {
  int const nsamples = 1 << 15;
  int const S = 20;
  int const V = 4;
  int const delay = 0;
  int const tol = 2;

  using array_type = jb::fftw::aligned_multi_array<float, 3>;
  using tested_type = jb::fftw::time_delay_estimator_many<array_type>;
  using confidence_type = typename tested_type::confidence_type;
  using argmax_type = typename tested_type::argmax_type;

  array_type a(boost::extents[S][V][nsamples]);
  array_type b(boost::extents[S][V][nsamples]);
  confidence_type confidence(a);
  // expected confidence result to compare within tolerance tol
  confidence_type expected_confidence(a);

  argmax_type argmax(a);
  // expected argmax result to compare within tolerance tol
  argmax_type expected_argmax(a);

  using sum2_type = jb::fftw::tde_result<array_type, float>;
  // sum of square value of timeseries
  sum2_type sum2(b);

  // construct the tested FTE
  tested_type tested(a, b);

  // fill the timeseries with triangle or square
  jb::testing::create_triangle_timeseries(nsamples, b);

  // a = delay shift timeseries b
  int count = 0;
  for (int i = 0; i != S; ++i) {
    for (int j = 0; j != V; ++j, ++count) {
      float acc_sum2 = 0;
      for (int k = 0; k != nsamples; ++k) {
        a[i][j][k] = b[i][j][(k + delay) % nsamples];
        acc_sum2 += a[i][j][k] * a[i][j][k];
      }
      sum2[count] = acc_sum2;
    }
  }

  // run the TDE...
  (void)tested.estimate_delay(a, b, sum2, confidence, argmax);
  // argmax should be around 0, which means possible around nsamples-1 also
  // so, let's "shift mod" argmax by a multiple of tol in order to make
  // close enough check simple to implement...
  count = 0;
  std::size_t shift = 10 * static_cast<std::size_t>(tol);
  for (int i = 0; i != S; ++i) {
    for (int j = 0; j != V; ++j, ++count) {
      expected_confidence[count] = static_cast<float>(nsamples);
      expected_argmax[count] = shift;
      argmax[count] = (argmax[count] + shift) % nsamples;
    }
  }

  // check argmax within delay +/- tol
  BOOST_CHECK_MESSAGE(
      jb::testing::tde_result_close_enough(argmax, expected_argmax, tol),
      "argmax is not within tolerance("
          << tol << ", argmax[0]=" << argmax[0]
          << ", expected_argmax[0]=" << expected_argmax[0]);
  // check confidence within nsamples +/- tol
  BOOST_CHECK_MESSAGE(
      jb::testing::tde_result_close_enough(
          confidence, expected_confidence, 10 * tol),
      "confidence is not within tolerance("
          << 10 * tol << "), confidence[0]=" << confidence[0]
          << ", expected_confidence[0]=" << expected_confidence[0]);
}
