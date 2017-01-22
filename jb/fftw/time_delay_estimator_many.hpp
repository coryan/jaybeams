#ifndef jb_fftw_time_delay_estimator_many_hpp
#define jb_fftw_time_delay_estimator_many_hpp

#include <jb/fftw/aligned_multi_array.hpp>
#include <jb/fftw/alignment_traits.hpp>

namespace jb {
namespace fftw {

/**
 * A simple time delay estimator based on cross-correlation.
 *
 * Timeseries are implemented as aligned multi arrays of N dimensions
 *
 * @tparam array_t timeseries multi array type
 * @tparam T timeseries value type
 * @tparam K timeseries dimensionality
 */

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
  using confidence_type = jb::fftw::tde_result<array_t, precision_type>;

  /// The type used to store the TDE argmax between two timeseries
  using argmax_type = jb::fftw::tde_result<array_t, std::size_t>;

  /// The sqr sum of a timeseries
  using sum2_type = jb::fftw::tde_result<array_t, precision_type>;

  //@}

  /**
   * Constructs members using the multi array characteristic a and b.
   *
   * @param a multi array timeries
   * @param b multi array timeries
   *
   */

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

  /**
   * Compute the time-delay estimate between two timeseries a and b.
   *
   * @param a input timeseries
   * @param b input timeseries
   * @param sum2 contains sqr sum of one of the timeseries (a or b)
   * @param confidence to return the confidence of the TDE(a,b)
   * @param argmax to return argmax TDE(a,b)
   */
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

private:
  /**
   * Determine the correct FFTW planning flags given the inputs.
   */
  static int planning_flags() {
    if (jb::fftw::always_aligned<array_type>::value) {
      return FFTW_MEASURE | FFTW_PRESERVE_INPUT;
    }
    return FFTW_MEASURE | FFTW_PRESERVE_INPUT | FFTW_UNALIGNED;
  }

private:
  /// tmpa_          : timeseries to store the result of FFT(a)
  frequency_array_type tmpa_;
  /// tmpb_          : timeseries to store the result of FFT(b)
  frequency_array_type tmpb_;
  /// a2tmpa_        : fftw plan to execute FFT(a)
  dplan a2tmpa_;
  /// b2tmpb_        : fftw plan to execute FFT(b)
  dplan b2tmpb_;
  /// out_           : timeseries to store the result of inverse FFT
  output_array_type out_;
  /// tmpa2out       : fftw plan to execute inverse FFT of a timeseries
  iplan tmpa2out_;
  /// nsamples       : num samples of timeseries
  std::size_t nsamples_;
  /// num_timeseries : num of timeseries contained in a and b
  std::size_t num_timeseries_;
};

} // namespace fftw
} // namespace jb

#endif // jb_fftw_time_delay_estimator_many_hpp
