#include <jb/fftw/aligned_multi_array.hpp>
#include <jb/fftw/alignment_traits.hpp>
#include <jb/fftw/plan.hpp>
#include <jb/testing/check_complex_close_enough.hpp>
#include <jb/testing/create_square_timeseries.hpp>
#include <jb/testing/create_triangle_timeseries.hpp>
#include <jb/testing/delay_timeseries.hpp>

#include <boost/test/unit_test.hpp>
#include <chrono>

namespace jb {
namespace testing {
template <typename T, typename U>
struct same_type {
  static const bool value = false;
};
template <typename T>
struct same_type<T, T> {
  static const bool value = true;
};
}
}

/**
 * Helpers to test jb::time_delay_estimator_many
 */
namespace {} // anonymous namespace

namespace jb {
namespace fftw {

/// Type used to store the TDE between two timeseries
/// handles TDE result for container type timeseries
template <typename container_t, typename value_t>
class tde_result {
public:
  using record_type = value_t;
  tde_result(container_t const& a)
      : value_{0} {
  }

  value_t& operator[](std::size_t pos) {
    return value_;
  }
  value_t const& operator[](std::size_t pos) const {
    return value_;
  }

private:
  record_type value_;
};

/// handles TDE result for array_type timeseries
template <typename T, std::size_t K, typename value_t>
class tde_result<jb::fftw::aligned_multi_array<T, K>, value_t> {
public:
  using array_type = jb::fftw::aligned_multi_array<T, K>;
  using record_type = jb::fftw::aligned_multi_array<value_t, K - 1>;

  tde_result(array_type const& a)
      : record_{jb::detail::array_shape(a)} {
  }

  value_t& operator[](std::size_t pos) {
    return record_.data()[pos];
  }
  value_t const& operator[](std::size_t pos) const {
    return record_.data()[pos];
  }

private:
  record_type record_;
};

/// handles TDE result for array_type dimensionality = 1 timeseries
template <typename T, typename value_t>
class tde_result<jb::fftw::aligned_multi_array<T, 1>, value_t> {
public:
  using array_type = jb::fftw::aligned_multi_array<T, 1>;
  using record_type = value_t;
  tde_result(array_type const& a)
      : value_{0} {
  }

  value_t& operator[](std::size_t pos) {
    return value_;
  }
  value_t const& operator[](std::size_t pos) const {
    return value_;
  }

private:
  record_type value_;
};

template <typename array_t, template <typename T, std::size_t K>
                            class multi_array = jb::fftw::aligned_multi_array>
class time_delay_estimator_many {
public:
  //@{

  /**
   * @name Type traits
   */
  /// The input timeseries type
  using array_type = array_t;

  /// The values stored in the input timeseries
  using element_type =
      typename jb::detail::array_traits<array_type>::element_type;

  /// Extract T out of std::complex<T>, otherwise simply T.
  using precision_type =
      typename jb::extract_value_type<element_type>::precision;

  /// The type used to store the DFT of the input timeseries
  using frequency_array_type =
      multi_array<std::complex<precision_type>, array_type::dimensionality>;

  /// The type used to store the inverse of the DFT
  using output_array_type =
      multi_array<precision_type, array_type::dimensionality>;

  /// The execution plan to apply the (forward) DFT
  using dplan = jb::fftw::plan<array_type, frequency_array_type>;

  /// The execution plan to apply the inverse (aka backward) DFT
  using iplan = jb::fftw::plan<frequency_array_type, output_array_type>;

  /// The type used to store the TDE confidence between two timeseries
  using confidence_type = tde_result<array_t, precision_type>;

  /// The type used to store the TDE argmax between two timeseries
  using argmax_type = tde_result<array_t, std::size_t>;

  /// The sqr sum of a timeseries
  using sum2_type = tde_result<array_t, precision_type>;

  //@}

  /// Constructor
  time_delay_estimator_many(array_type const& a, array_type const& b)
      : tmpa_{jb::detail::array_shape(a)}
      , tmpb_{jb::detail::array_shape(b)}
      , a2tmpa_{create_forward_plan(a, tmpa_, planning_flags())}
      , b2tmpb_{create_forward_plan(b, tmpb_, planning_flags())}
      , out_{jb::detail::array_shape(a)}
      , tmpa2out_{create_backward_plan(tmpa_, out_, planning_flags())}
      , nsamples_{jb::detail::nsamples(a)}
      , num_timeseries_{jb::detail::element_count(a) / nsamples_} {
    if (a.size() != b.size()) {
      throw std::invalid_argument("size mismatch in time_delay_estimator ctor");
    }
  }

  /// Compute the time-delay estimate between two timeseries
  void estimate_delay(
      array_type const& a, array_type const& b, sum2_type const& sum2,
      confidence_type& confidence, argmax_type& argmax) {
    // Validate the input sizes.  For some types of timeseries the
    // alignment may be different too, but we only use the alignment
    // when the type of timeseries guarantees to always be aligned.
    if (a.size() != tmpa_.size() or b.size() != tmpa_.size()) {
      throw std::invalid_argument(
          "size mismatch in time_delay_estimator<>::estimate_delay()");
    }
    // First we apply the Fourier transform to both inputs ...
    a2tmpa_.execute(a, tmpa_);
    b2tmpb_.execute(b, tmpb_);
    // ... then we compute Conj(A) * B for the transformed inputs ...
    for (std::size_t i = 0; i != tmpa_.num_elements(); ++i) {
      tmpa_.data()[i] = std::conj(tmpa_.data()[i]) * (tmpb_.data()[i]);
    }
    // ... then we compute the inverse Fourier transform to the result ...
    tmpa2out_.execute(tmpa_, out_);

    // ... finally we compute (argmax,max) for the result(s) ...
    std::size_t k = 0;
    for (std::size_t i = 0; i != num_timeseries_; ++i) {
      precision_type max_val = std::numeric_limits<precision_type>::min();
      std::size_t argmax_val = 0;
      for (std::size_t j = 0; j != nsamples_; ++j, ++k) {
        if (max_val < out_.data()[k]) {
          max_val = out_.data()[k];
          argmax_val = j;
        }
      }
      confidence[i] = max_val;
      argmax[i] = argmax_val;
    }
  }

  precision_type best_estimate_delay(
      confidence_type const& confidence, argmax_type const& argmax) {
    precision_type accumulator = 0;
    precision_type weight = 0;
    for (std::size_t i = 0; i != num_timeseries_; ++i) {
      accumulator += argmax[i] * confidence[i];
      weight += confidence[i];
    }
    return accumulator / weight;
  }

private:
  /**
   * Determine the correct FFTW planning flags given the inputs.
   */
  static int planning_flags() {
    if (jb::fftw::always_aligned<array_type>::value) {
      return FFTW_MEASURE;
    }
    return FFTW_MEASURE | FFTW_UNALIGNED;
  }

private:
  frequency_array_type tmpa_;
  frequency_array_type tmpb_;
  dplan a2tmpa_;
  dplan b2tmpb_;
  output_array_type out_;
  iplan tmpa2out_;
  std::size_t nsamples_;
  std::size_t num_timeseries_;
};

} // namespace fftw
} // namespace jb

/**
 * @test Verify that jb::fftw::time_delay_estimator_many dimension = 1
 */
BOOST_AUTO_TEST_CASE(fftw_time_delay_estimator_dim_1) {
  int const nsamples = 10000;
  int const delay = 200;

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

  // check types
  bool res = jb::testing::same_type<confidence_type::record_type, float>::value;
  BOOST_CHECK_MESSAGE(res == true, "confidence record_type is not float");
  res = jb::testing::same_type<argmax_type::record_type, std::size_t>::value;
  BOOST_CHECK_MESSAGE(res == true, " argmax record_type is not size_t");
  res = jb::testing::same_type<sum2_type::record_type, float>::value;
  BOOST_CHECK_MESSAGE(res == true, "sum2 record_type is not float");
  res = jb::testing::same_type<tested_type::precision_type, float>::value;
  BOOST_CHECK_MESSAGE(res == true, "tested precision_type is not float");

  jb::testing::create_triangle_timeseries(nsamples, b);
  float acc_sum2 = 0;
  for (int i = 0; i != nsamples; ++i) {
    a[i] = b[(i + delay) % nsamples];
    acc_sum2 += a[i] * a[i];
  }
  sum2[0] = acc_sum2;

  tested_type tested(a, b);
  (void)tested.estimate_delay(a, b, sum2, confidence, argmax);

  BOOST_CHECK_MESSAGE(argmax[0] == 2, "argmax = " << argmax[0]);
}

/**
 * @test Verify that we can create and use a simple time delay estimator.
 */
BOOST_AUTO_TEST_CASE(fftw_time_delay_estimator_many_simple) {
  //  int const nsamples = 1 << 15;
  int const nsamples = 1000;
  int const delay = 25;
  int const S = 4;
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

  // check types
  using test_array_type = jb::fftw::aligned_multi_array<float, 1>;
  using test_argmax_array_type = jb::fftw::aligned_multi_array<std::size_t, 1>;
  bool res = jb::testing::same_type<confidence_type::record_type,
                                    test_array_type>::value;
  BOOST_CHECK_MESSAGE(
      res == true, "confidence record_type is not multi array float");
  res = jb::testing::same_type<argmax_type::record_type,
                               test_argmax_array_type>::value;
  BOOST_CHECK_MESSAGE(res == true, " argmax record_type is not multi array");
  res = jb::testing::same_type<sum2_type::record_type, test_array_type>::value;
  BOOST_CHECK_MESSAGE(res == true, "sum2 record_type is not multi array float");
  res = jb::testing::same_type<tested_type::precision_type, float>::value;
  BOOST_CHECK_MESSAGE(res == true, "tested precision_type is not float");

  int count = 0;
  for (auto vector : b) {
    if (++count % 2 == 0) {
      jb::testing::create_triangle_timeseries(nsamples, vector);
    } else {
      jb::testing::create_square_timeseries(nsamples, vector);
    }
  }
  for (int i = 0; i != S; ++i) {
    float acc_sum2 = 0;
    for (int j = 0; j != nsamples; ++j) {
      a[i][j] = b[i][(j + delay) % nsamples];
      acc_sum2 += a[i][j] * a[i][j];
    }
    sum2[i] = acc_sum2;
  }

  tested_type tested(a, b);
  (void)tested.estimate_delay(a, b, sum2, confidence, argmax);
  for (int i = 0; i != S; ++i) {
    BOOST_CHECK_MESSAGE(
        argmax[i] == delay, "argmax[" << i << "] = " << argmax[i]);
  }
  //  auto estimate_delay = tested.best_estimate_delay(confidence, argmax);
  //  BOOST_CHECK_CLOSE(estimate_delay, 0.123456); // why not, right?
}

/**
 * @test Verify that we can create and use a simple time delay estimator.
 */
BOOST_AUTO_TEST_CASE(fftw_time_delay_estimator_many_just_checking) {
  //  int const nsamples = 1 << 15;
  int const nsamples = 100;
  int const S = 20;
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

  for (std::size_t j = 0; j != a.num_elements(); ++j) {
    a.data()[j] = 100 + j;
    b.data()[j] = 200 + j;
  }

  int k = 0;
  for (int i = 0; i != S; ++i) {
    for (int j = 0; j != nsamples; ++j) {
      BOOST_CHECK_MESSAGE(
          a[i][j] == 100 + k, "a[" << i << "][" << j << "] = " << a[i][j]);
      BOOST_CHECK_MESSAGE(
          b[i][j] == 200 + k, "b[" << i << "][" << j << "] = " << b[i][j]);
      k++;
    }
  }
}

/**
 * @test Verify that we can create and use a simple time delay estimator.
 */
BOOST_AUTO_TEST_CASE(fftw_time_delay_estimator_many_just_checking_dim_1) {
  //  int const nsamples = 1 << 15;
  int const nsamples = 10;
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

  for (int j = 0; j != nsamples; ++j) {
    a.data()[j] = 100 + j;
    b.data()[j] = 200 + j;
  }
  sum2[0] = 1;
  BOOST_CHECK_MESSAGE(sum2[0] == 1, "sum2[0] = " << sum2[0]);
  confidence[0] = 2;
  BOOST_CHECK_MESSAGE(confidence[0] == 2, "confidence[0] = " << confidence[0]);
  argmax[0] = 3;
  BOOST_CHECK_MESSAGE(argmax[0] == 3, "argmax[0] = " << argmax[0]);

  for (int j = 0; j != nsamples; ++j) {
    BOOST_CHECK_MESSAGE(a[j] == 100 + j, "a[" << j << "] = " << a[j]);
    BOOST_CHECK_MESSAGE(b[j] == 200 + j, "b[" << j << "] = " << b[j]);
  }
}
