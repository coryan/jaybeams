#ifndef jb_fftw_time_delay_estimator_many_hpp
#define jb_fftw_time_delay_estimator_many_hpp

#include <jb/detail/array_traits.hpp>
#include <jb/fftw/plan.hpp>
#include <jb/fftw/tde_result.hpp>
#include <jb/complex_traits.hpp>

/*
#include <jb/testing/check_close_enough.hpp>
*/

namespace jb {
namespace fftw {

/**
 * A time delay estimator based on cross-correlation.
 *
 * Timeseries are implemented as standard containers (e.g. vector<>), as well as
 * boost multi arrays of N dimensions
 *
 * @tparam array_t timeseries array type
 */
template <typename array_t>
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

  /// complex_type use by FFT plans frequency array type
  using complex_type = std::complex<precision_type>;

  /// The type used to store the DFT of the input timeseries
  using frequency_array_type =
      typename jb::detail::aligned_container<complex_type,
                                             array_type>::array_type;

  /// The type used to store the inverse of the DFT
  using output_array_type =
      typename jb::detail::aligned_container<precision_type,
                                             array_type>::array_type;

  /// The execution plan to apply the (forward) DFT
  using dplan = jb::fftw::plan<array_type, frequency_array_type>;

  /// The execution plan to apply the inverse (aka backward) DFT
  using iplan = jb::fftw::plan<frequency_array_type, output_array_type>;

  /// The type used to store the TDE confidence between two timeseries
  using confidence_type = jb::fftw::tde_result<array_t, precision_type>;

  /// The type used to store the estimated_delay between two timeseries
  using estimated_delay_type = jb::fftw::tde_result<array_t, std::size_t>;

  /// The sqr sum of a timeseries
  using sum2_type = jb::fftw::tde_result<array_t, precision_type>;

  //@}

  /**
   * Constructs a time delay estimator using @a a and @a b as prototypes
   * for the arguments.
   *
   * The optimal algorithm to compute the  FFTs used in the cross correlation
   * depends on the size of the input parameters and their memory alignment.
   *
   * The FFTW library modifies the arguments to compute the optimal
   * execution plan, do not assume the values are unmodified.
   *
   * @param a multi array timeseries
   * @param b multi array timeseries
   */
  time_delay_estimator_many(array_type& a, array_type& b)
      : tmpa_(jb::detail::array_shape(a))
      , tmpb_(jb::detail::array_shape(b))
      , a2tmpa_(create_forward_plan(a, tmpa_, planning_flags()))
      , b2tmpb_(create_forward_plan(b, tmpb_, planning_flags()))
      , out_(jb::detail::array_shape(a))
      , tmpa2out_(create_backward_plan(tmpa_, out_, planning_flags()))
      , nsamples_(jb::detail::nsamples(a))
      , num_timeseries_(jb::detail::element_count(a) / nsamples_) {
    if (a.size() != b.size()) {
      throw std::invalid_argument("size mismatch in time_delay_estimator ctor");
    }
  }

  /**
   * Compute the time-delay estimate between two timeseries a and b.
   *
   * @param confidence to return the TDE(a,b) confidence
   * @param estimated_delay to return the result of TDE(a,b)
   * @param a input timeseries, FFTW library might modify their values
   * @param b input timeseries, FFTW library might modify their values
   * @param sum2 contains sqr sum of one of the timeseries (a or b)
   */
  void estimate_delay(
      confidence_type& confidence, estimated_delay_type& estimated_delay,
      array_type& a, array_type& b, sum2_type const& sum2) {
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
    // @todo issue #86: investigate use of SSE instructions
    complex_type* it_tmpa = tmpa_.data();
    complex_type* it_tmpb = tmpb_.data();
    for (std::size_t i = 0; i != jb::detail::element_count(tmpa_);
         ++i, ++it_tmpa, ++it_tmpb) {
      *it_tmpa = std::conj(*it_tmpa) * (*it_tmpb);
    }
    // ... then we compute the inverse Fourier transform to the result ...
    tmpa2out_.execute(tmpa_, out_);

    // ... finally we compute the estimated delay and its confidence
    // @todo issue #86: investigate use of SSE instructions
    precision_type* it_out = out_.data();
    for (std::size_t i = 0; i != num_timeseries_; ++i) {
      precision_type max_val = std::numeric_limits<precision_type>::min();
      std::size_t tde_val = 0;
      for (std::size_t j = 0; j != nsamples_; ++j, ++it_out) {
        if (max_val < *it_out) {
          max_val = *it_out;
          tde_val = j;
        }
      }
      if (sum2[i] < std::numeric_limits<precision_type>::epsilon()) {
        confidence[i] = std::numeric_limits<precision_type>::max();
      } else {
        confidence[i] = max_val / sum2[i];
      }
      estimated_delay[i] = tde_val;
    }
  }

private:
  /**
   * Determine the correct FFTW planning flags given the inputs.
   */
  static int planning_flags() {
    if (jb::detail::always_aligned<array_type>::value) {
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
