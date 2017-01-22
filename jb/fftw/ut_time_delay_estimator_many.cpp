#include <jb/fftw/plan.hpp>
#include <jb/fftw/tde_result.hpp>
#include <jb/fftw/time_delay_estimator_many.hpp>
#include <jb/testing/check_complex_close_enough.hpp>
#include <jb/testing/create_square_timeseries.hpp>
#include <jb/testing/create_triangle_timeseries.hpp>
#include <jb/testing/delay_timeseries.hpp>

#include <boost/test/unit_test.hpp>
#include <chrono>

/**
 * @test Verify that we can create and use a dimension 3
 * time delay estimator type.
 */
BOOST_AUTO_TEST_CASE(fftw_time_delay_estimator_many_3_dim_usage) {
  int const nsamples = 1 << 15;
  int const S = 20;
  int const V = 4;
  using array_type = jb::fftw::aligned_multi_array<float, 3>;
  using tested_type = jb::fftw::time_delay_estimator_many<array_type>;

  array_type a(boost::extents[S][V][nsamples]);
  array_type b(boost::extents[S][V][nsamples]);
  // check if we can call the constructor
  tested_type tested(a, b);

  // check confidence tde_result type
  using test_confidence_type = jb::fftw::aligned_multi_array<float, 2>;
  using confidence_type = typename tested_type::confidence_type;
  bool res =
      std::is_same<confidence_type::record_type, test_confidence_type>::value;
  BOOST_CHECK_MESSAGE(
      res == true, "confidence record_type is not multi array float");
  confidence_type confidence(a);

  // check argmax tde_result type
  using test_argmax_type = jb::fftw::aligned_multi_array<std::size_t, 2>;
  using argmax_type = typename tested_type::argmax_type;
  res = std::is_same<argmax_type::record_type, test_argmax_type>::value;
  BOOST_CHECK_MESSAGE(
      res == true, " argmax record_type is not multi array size_t");
  argmax_type argmax(a);

  // fill the timeseries and tde_results
  std::size_t counter = 0;
  for (int i = 0; i != S; ++i) {
    for (int j = 0; j != V; ++j) {
      int pos = counter / nsamples;
      confidence[pos] = (float)counter;
      argmax[pos] = counter;
      for (int k = 0; k != nsamples; ++k) {
        a[i][j][k] = counter++;
      }
    }
  }

  // ... now check the values using .data()[counter] access
  for (std::size_t i = 0; i != a.num_elements(); ++i) {
    BOOST_CHECK_MESSAGE(a.data()[i] == i, "a.data()[" << i << "] != " << i);
    if (not i % nsamples) {
      int pos = i / nsamples;
      BOOST_CHECK_MESSAGE(argmax[pos] == i, "argmax[" << pos << "] != " << i);
      BOOST_CHECK_MESSAGE(
          confidence[pos] == (float)i, "confidence[" << pos << "] != " << i);
    }
  }
}

/**
 * @test Verify that we can create and use a dimension 2
 * time delay estimator type.
 */
BOOST_AUTO_TEST_CASE(fftw_time_delay_estimator_many_2_dim_usage) {
  int const nsamples = 1 << 15;
  int const S = 20;
  using array_type = jb::fftw::aligned_multi_array<float, 2>;
  using tested_type = jb::fftw::time_delay_estimator_many<array_type>;

  array_type a(boost::extents[S][nsamples]);
  array_type b(boost::extents[S][nsamples]);
  // check if we can call the constructor
  tested_type tested(a, b);

  // check confidence tde_result type
  using test_confidence_type = jb::fftw::aligned_multi_array<float, 1>;
  using confidence_type = typename tested_type::confidence_type;
  bool res =
      std::is_same<confidence_type::record_type, test_confidence_type>::value;
  BOOST_CHECK_MESSAGE(
      res == true, "confidence record_type is not multi array float");
  confidence_type confidence(a);

  // check argmax tde_result type
  using test_argmax_type = jb::fftw::aligned_multi_array<std::size_t, 1>;
  using argmax_type = typename tested_type::argmax_type;
  res = std::is_same<argmax_type::record_type, test_argmax_type>::value;
  BOOST_CHECK_MESSAGE(
      res == true, " argmax record_type is not multi array size_t");
  argmax_type argmax(a);

  // fill the timeseries and tde_results
  std::size_t counter = 0;
  for (int i = 0; i != S; ++i) {
    int pos = counter / nsamples;
    confidence[pos] = (float)counter;
    argmax[pos] = counter;
    for (int k = 0; k != nsamples; ++k) {
      a[i][k] = counter++;
    }
  }

  // ... now check the values using .data()[counter] access
  for (std::size_t i = 0; i != a.num_elements(); ++i) {
    BOOST_CHECK_MESSAGE(a.data()[i] == i, "a.data()[" << i << "] != " << i);
    if (not i % nsamples) {
      int pos = i / nsamples;
      BOOST_CHECK_MESSAGE(argmax[pos] == i, "argmax[" << pos << "] != " << i);
      BOOST_CHECK_MESSAGE(
          confidence[pos] == (float)i, "confidence[" << pos << "] != " << i);
    }
  }
}

/**
 * @test Verify that we can create and use a dimension 1
 * time delay estimator type.
 */
BOOST_AUTO_TEST_CASE(fftw_time_delay_estimator_many_1_dim_usage) {
  int const nsamples = 1 << 15;
  using array_type = jb::fftw::aligned_multi_array<float, 1>;
  using tested_type = jb::fftw::time_delay_estimator_many<array_type>;

  array_type a(boost::extents[nsamples]);
  array_type b(boost::extents[nsamples]);
  // check if we can call the constructor
  tested_type tested(a, b);

  // check confidence tde_result type
  using test_confidence_type = float;
  using confidence_type = typename tested_type::confidence_type;
  bool res =
      std::is_same<confidence_type::record_type, test_confidence_type>::value;
  BOOST_CHECK_MESSAGE(res == true, "confidence record_type is not float");
  confidence_type confidence(a);

  // check argmax tde_result type
  using test_argmax_type = std::size_t;
  using argmax_type = typename tested_type::argmax_type;
  res = std::is_same<argmax_type::record_type, test_argmax_type>::value;
  BOOST_CHECK_MESSAGE(res == true, " argmax record_type is not size_t");
  argmax_type argmax(a);

  // fill the timeseries and tde_results
  std::size_t counter = 0;
  confidence[counter] = (float)counter;
  argmax[counter] = counter;
  for (int k = 0; k != nsamples; ++k) {
    a[k] = counter++;
  }

  // ... now check the values using .data()[counter] access
  for (std::size_t i = 0; i != a.num_elements(); ++i) {
    BOOST_CHECK_MESSAGE(a.data()[i] == i, "a.data()[" << i << "] != " << i);
    if (not i % nsamples) {
      int pos = i / nsamples;
      BOOST_CHECK_MESSAGE(argmax[pos] == i, "argmax[" << pos << "] != " << i);
      BOOST_CHECK_MESSAGE(
          confidence[pos] == (float)i, "confidence[" << pos << "] != " << i);
    }
  }
}

/**
 * @test Verify that we can create and use a simple time delay estimator.
 * dimension 3
 */
BOOST_AUTO_TEST_CASE(fftw_time_delay_estimator_many_3_dim_tde) {
  int const nsamples = 1 << 15;
  int const S = 20;
  int const V = 4;
  int const delay = 250;
  using array_type = jb::fftw::aligned_multi_array<float, 3>;
  using tested_type = jb::fftw::time_delay_estimator_many<array_type>;
  using confidence_type = typename tested_type::confidence_type;
  using argmax_type = typename tested_type::argmax_type;

  array_type a(boost::extents[S][V][nsamples]);
  array_type b(boost::extents[S][V][nsamples]);
  confidence_type confidence(a);
  argmax_type argmax(a);

  using sum2_type = jb::fftw::tde_result<array_type, float>;
  sum2_type sum2(b);

  // construct the tested FTE
  tested_type tested(a, b);

  // fill the timeseries with triangle or square
  jb::testing::create_triangle_timeseries(nsamples, b);

  // a = delay shift timeseries b
  int count = 0;
  for (int i = 0; i != S; ++i) {
    for (int j = 0; j != V; ++j) {
      float acc_sum2 = 0;
      for (int k = 0; k != nsamples; ++k) {
        a[i][j][k] = b[i][j][(k + delay) % nsamples];
        acc_sum2 += a[i][j][k] * a[i][j][k];
      }
      sum2[count++] = acc_sum2;
    }
  }

  count = 0;
  (void)tested.estimate_delay(a, b, sum2, confidence, argmax);
  for (int i = 0; i != S; ++i) {
    for (int j = 0; j != V; ++j) {
      BOOST_CHECK_MESSAGE(
          argmax[count] == delay, "argmax[" << count
                                            << "] = " << argmax[count]);
      count++;
    }
  }
}

/**
 * @test Verify that we can create and use a simple time delay estimator.
 * dimension 2
 */
BOOST_AUTO_TEST_CASE(fftw_time_delay_estimator_many_2_dim_tde) {
  int const nsamples = 1 << 15;
  int const S = 20;
  int const delay = 250;
  using array_type = jb::fftw::aligned_multi_array<float, 2>;
  using tested_type = jb::fftw::time_delay_estimator_many<array_type>;
  using confidence_type = typename tested_type::confidence_type;
  using argmax_type = typename tested_type::argmax_type;

  array_type a(boost::extents[S][nsamples]);
  array_type b(boost::extents[S][nsamples]);
  confidence_type confidence(a);
  argmax_type argmax(a);

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
    sum2[count++] = acc_sum2;
  }

  (void)tested.estimate_delay(a, b, sum2, confidence, argmax);
  for (int i = 0; i != S; ++i) {
    BOOST_CHECK_MESSAGE(
        argmax[i] == delay, "argmax[" << i << "] = " << argmax[i]);
  }
}

/**
 * @test Verify that we can create and use a simple time delay estimator.
 * dimension 1
 */
BOOST_AUTO_TEST_CASE(fftw_time_delay_estimator_many_1_dim_tde) {
  int const nsamples = 1 << 15;
  int const delay = 250;
  using array_type = jb::fftw::aligned_multi_array<float, 1>;
  using tested_type = jb::fftw::time_delay_estimator_many<array_type>;
  using confidence_type = typename tested_type::confidence_type;
  using argmax_type = typename tested_type::argmax_type;

  array_type a(boost::extents[nsamples]);
  array_type b(boost::extents[nsamples]);
  confidence_type confidence(a);
  argmax_type argmax(a);

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

  (void)tested.estimate_delay(a, b, sum2, confidence, argmax);
  BOOST_CHECK_MESSAGE(argmax[0] == delay, "argmax = " << argmax[0]);
}
