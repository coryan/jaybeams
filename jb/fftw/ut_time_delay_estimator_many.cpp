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

  /// The type used to stored the inverse of the DFT
  using output_array_type =
      multi_array<precision_type, array_type::dimensionality>;

  /// The execution plan to apply the (forward) DFT
  using dplan = jb::fftw::plan<array_type, frequency_array_type>;

  /// The execution plan to apply the inverse (aka backward) DFT
  using iplan = jb::fftw::plan<frequency_array_type, output_array_type>;
  //@}

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
  std::vector<std::pair<bool, precision_type>>
  estimate_delay(array_type const& a, array_type const& b) {
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
    // ... finally we compute (argmax,max) for the result ...
    precision_type max = std::numeric_limits<precision_type>::min();
    std::size_t argmax = 0;
    for (std::size_t i = 0; i != out_.size(); ++i) {
      if (max < out_[i]) {
        max = out_[i];
        argmax = i;
      }
    }
    // TODO(#13) the threshold for acceptance should be configurable,
    // maybe we want the value to be close to the expected area of 'a'
    // for example ...
    if (max <= std::numeric_limits<precision_type>::epsilon()) {
      return std::make_pair(false, precision_type(0));
    }
    return std::make_pair(true, precision_type(argmax));
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

  array_type a(boost::extents[S][nsamples]);
  array_type b(boost::extents[S][nsamples]);
  int count = 0;
  for (auto vector : a) {
    if (++count % 2 == 0) {
      jb::testing::create_triangle_timeseries(nsamples, vector);
    } else {
      jb::testing::create_square_timeseries(nsamples, vector);
    }
  }
  for (int i = 0; i != S; ++i) {
    for (int j = 0; j != nsamples; ++j) {
      b[i][j] = a[i][(j + delay) % nsamples];
    }
  }
  tested_type tested(a, b);
}
