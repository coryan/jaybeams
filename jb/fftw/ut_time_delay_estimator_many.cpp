#include <jb/fftw/aligned_multi_array.hpp>
#include <jb/fftw/alignment_traits.hpp>
#include <jb/fftw/plan.hpp>
#include <jb/testing/create_square_timeseries.hpp>
#include <jb/testing/create_triangle_timeseries.hpp>
#include <jb/testing/delay_timeseries.hpp>

#include <boost/test/unit_test.hpp>
#include <chrono>

/**
 * Helpers to test jb::time_delay_estimator_many
 */
namespace {} // anonymous namespace

namespace jb {
namespace fftw {

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

  /// TDE base result type
  /// size_t is the arg max of the cross correlation
  /// precision_type is the max of the cross correlation
  using tde_base_result_type = std::pair<std::size_t, precision_type>;

  //@}

  /// Type used to store the TDE between two timeseries
  /// handles TDE result for container type timeseries
  template <typename container_type>
  struct tde_result {
    using element_type = tde_base_result_type;

    tde_result(container_type const& a)
        : nsamples_{a.nsamples()}
        , num_timeseries_{1}
        , result_{0} {
    }

    std::size_t nsamples() const {
      return nsamples_;
    }
    std::size_t num_timeseries() const {
      return num_timeseries_;
    }
    std::size_t getargmax(std::size_t i) const {
      return std::get<0>(result_);
    }
    precision_type getmax(std::size_t i) const {
      return std::get<1>(result_);
    }

    void push(std::size_t argmax, precision_type max) {
      result_ = std::make_pair(argmax, max);
    }

  private:
    std::size_t nsamples_;
    std::size_t num_timeseries_;
    element_type result_;
  };

  /// handles TDE result for array_type timeseries
  template <typename T, std::size_t K, typename A>
  struct tde_result<boost::multi_array<T, K, A>> {
    using element_type = boost::multi_array<tde_base_result_type,
                                            array_type::dimensionality - 1>;

    tde_result(boost::multi_array<T, K, A> const& a)
        : nsamples_{jb::detail::nsamples(a)}
        , num_timeseries_{jb::detail::element_count(a) / nsamples_}
        , result_{jb::detail::array_shape(a)}
        , pos_{0} {
    }

    void push(std::size_t argmax, precision_type max) {
      result_.data()[pos_++] = std::make_pair(argmax, max);
    }

    std::size_t nsamples() const {
      return nsamples_;
    }
    std::size_t num_timeseries() const {
      return num_timeseries_;
    }
    std::size_t getargmax(std::size_t i) const {
      return std::get<0>(result_.data()[i]);
    }
    precision_type getmax(std::size_t i) const {
      return std::get<1>(result_.data()[i]);
    }

  private:
    std::size_t nsamples_;
    std::size_t num_timeseries_;
    element_type result_;
    std::size_t pos_;
  };

  /// handles TDE result for array_type dimensionality = 2 timeseries
  template <typename T, typename A>
  struct tde_result<boost::multi_array<T, 2, A>> {
    using element_type = std::vector<tde_base_result_type>;

    tde_result(boost::multi_array<T, 2, A> const& a)
        : nsamples_{jb::detail::nsamples(a)}
        , num_timeseries_{jb::detail::element_count(a) / nsamples_}
        , result_{element_type(num_timeseries_)}
        , pos_{0} {
    }

    std::size_t nsamples() const {
      return nsamples_;
    }
    std::size_t num_timeseries() const {
      return num_timeseries_;
    }
    std::size_t getargmax(std::size_t i) const {
      return std::get<0>(result_[i]);
    }
    precision_type getmax(std::size_t i) const {
      return std::get<1>(result_[i]);
    }

    void push(std::size_t argmax, precision_type max) {
      result_[pos_++] = std::make_pair(argmax, max);
    }

  private:
    std::size_t nsamples_;
    std::size_t num_timeseries_;
    element_type result_;
    std::size_t pos_;
  };

  /// Type used to accumulate a characteristic of a timeseries
  /// handles accumulate for container type timeseries
  template <typename container_type>
  struct accumulator {
    using element_type = precision_type;

    accumulator(container_type const& a)
        : num_timeseries_{1}
        , result_{0} {
    }

    std::size_t num_timeseries() const {
      return num_timeseries_;
    }
    precision_type getdata(std::size_t pos) const {
      return result_;
    }

    void push(precision_type value) {
      result_ = value;
    }

  private:
    std::size_t num_timeseries_;
    element_type result_;
  };

  /// handles accumulator for array_type timeseries
  template <typename T, std::size_t K, typename A>
  struct accumulator<boost::multi_array<T, K, A>> {
    using element_type =
        boost::multi_array<precision_type, array_type::dimensionality - 1>;

    accumulator(boost::multi_array<T, K, A> const& a)
        : num_timeseries_{jb::detail::element_count(a) /
                          jb::detail::nsamples(a)}
        , result_{jb::detail::array_shape(a)}
        , pos_{0} {
    }

    void push(precision_type value) {
      result_.data()[pos_++] = value;
    }

    std::size_t num_timeseries() const {
      return num_timeseries_;
    }
    precision_type getdata(std::size_t pos) const {
      return result_.data()[pos];
    }

  private:
    std::size_t num_timeseries_;
    element_type result_;
    std::size_t pos_;
  };

  /// handles accumulator for array_type dimensionality = 2 timeseries
  template <typename T, typename A>
  struct accumulator<boost::multi_array<T, 2, A>> {
    using element_type = std::vector<precision_type>;

    accumulator(boost::multi_array<T, 2, A> const& a)
        : num_timeseries_{jb::detail::element_count(a) /
                          jb::detail::nsamples(a)}
        , result_{element_type(num_timeseries_)}
        , pos_{0} {
    }

    std::size_t num_timeseries() const {
      return num_timeseries_;
    }
    precision_type getdata(std::size_t pos) const {
      return result_[pos];
    }

    void push(precision_type value) {
      result_[pos_++] = value;
    }

  private:
    std::size_t num_timeseries_;
    element_type result_;
    std::size_t pos_;
  };

  /// The base type used to store the TDE between two timeseries
  using tde_type = tde_result<array_type>;

  /// The base type used to accumulate sum sqr of a timeseries
  using sum2_type = accumulator<array_type>;

  /// Constructor
  time_delay_estimator_many(array_type const& a, array_type const& b)
      : tmpa_(jb::detail::array_shape(a))
      , tmpb_(jb::detail::array_shape(b))
      , a2tmpa_(create_forward_plan(a, tmpa_, planning_flags()))
      , b2tmpb_(create_forward_plan(b, tmpb_, planning_flags()))
      , out_(jb::detail::array_shape(a))
      , tmpa2out_(create_backward_plan(tmpa_, out_, planning_flags())) {
    if (a.size() != b.size()) {
      throw std::invalid_argument("size mismatch in time_delay_estimator ctor");
    }
  }

  /// Compute the time-delay estimate between two timeseries
  tde_type estimate_delay(array_type const& a, array_type const& b) {
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
      tmpa_.data()[i] = std::conj(tmpa_.data()[i]) * tmpb_.data()[i];
    }
    // ... then we compute the inverse Fourier transform to the result ...
    tmpa2out_.execute(tmpa_, out_);

    // ... finally we compute (argmax,max) for the result(s) ...
    tde_type tde(out_);
    std::size_t k = 0;
    for (std::size_t i = 0; i != tde.num_timeseries(); ++i) {
      precision_type max = std::numeric_limits<precision_type>::min();
      std::size_t argmax = 0;
      for (std::size_t j = 0; j != tde.nsamples(); ++j, ++k) {
        if (max < out_.data()[k]) {
          max = out_.data()[k];
          argmax = j;
        }
      }
      tde.push(argmax, max);
    }
    return tde;
  }

  precision_type
  best_estimate_delay(tde_type const& tde, sum2_type const& sum2) {
    precision_type accumulator = 0;
    precision_type weight = 0;
    for (std::size_t i = 0; i != tde.num_timeseries(); ++i) {
      precision_type confidence = tde.getmax(i) / sum2.getdata(i);
      accumulator += tde.getargmax(i) * confidence;
      weight += confidence;
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
};

} // namespace fftw
} // namespace jb

/**
 * @test Verify that we can create and use a simple time delay estimator.
 */
BOOST_AUTO_TEST_CASE(fftw_time_delay_estimator_many_simple) {
  int const nsamples = 1 << 15;
  int const delay = 1250;
  int const S = 128;
  using array_type = jb::fftw::aligned_multi_array<float, 2>;
  using tested_type = jb::fftw::time_delay_estimator_many<array_type>;
  using sum2_type = typename tested_type::sum2_type;

  array_type a(boost::extents[S][nsamples]);
  array_type b(boost::extents[S][nsamples]);
  sum2_type sum2(a);

  int count = 0;
  for (auto vector : a) {
    if (++count % 2 == 0) {
      jb::testing::create_triangle_timeseries(nsamples, vector);
    } else {
      jb::testing::create_square_timeseries(nsamples, vector);
    }
  }
  for (int i = 0; i != S; ++i) {
    float sum = 0;
    for (int j = 0; j != nsamples; ++j) {
      b[i][j] = a[i][(j + delay) % nsamples];
      sum += a[i][j] * a[i][j];
    }
    sum2.push(sum);
  }

  tested_type tested(a, b);
  auto res = tested.estimate_delay(a, b);
  auto estimate_delay = tested.best_estimate_delay(res, sum2);
  BOOST_CHECK_EQUAL(estimate_delay, 0.123456); // why not, right?
}
