#ifndef jb_fftw_time_delay_estimator_hpp
#define jb_fftw_time_delay_estimator_hpp

#include <jb/detail/array_traits.hpp>
#include <jb/fftw/plan.hpp>
#include <jb/complex_traits.hpp>

namespace jb {
namespace fftw {

/**
 * A simple time delay estimator based on cross-correlation
 */
template <typename timeseries_t,
          template <typename T> class vector = jb::fftw::aligned_vector>
class time_delay_estimator {
public:
  //@{
  /**
   * @name Type traits
   */
  /// The input timeseries type
  typedef timeseries_t timeseries_type;

  /// The values stored in the input timeseries
  typedef typename timeseries_type::value_type value_type;

  /// Extract T out of std::complex<T>, otherwise simply T.
  typedef typename jb::extract_value_type<value_type>::precision precision_type;

  /// The type used to store the DFT of the input timeseries
  typedef vector<std::complex<precision_type>> frequency_timeseries_type;

  /// The type used to stored the inverse of the DFT
  typedef vector<precision_type> output_timeseries_type;

  /// The execution plan to apply the (forward) DFT
  typedef jb::fftw::plan<timeseries_type, frequency_timeseries_type> dplan;

  /// The execution plan to apply the inverse (aka backward) DFT
  typedef jb::fftw::plan<frequency_timeseries_type, output_timeseries_type>
      iplan;
  //@}

  /**
   * Constructs a time delay estimator using @param a and @param b as prototypes
   * for the arguments.
   *
   * The optimal algorithm to compute the  FFTs used in the cross correlation
   * depends on the size of the input parameters and their memory alignment.
   *
   * The FFTW library modifies the arguments to compute the optimal
   * execution plan, do not assume the values are unmodified.
   *
   * @param a container type (e.g. vector<>) timeseries
   * @param b container type (e.g. vector<>) timeseries
   */
  time_delay_estimator(timeseries_type& a, timeseries_type& b)
      : tmpa_(a.size())
      , tmpb_(b.size())
      , a2tmpa_(create_forward_plan(a, tmpa_, planning_flags()))
      , b2tmpb_(create_forward_plan(b, tmpb_, planning_flags()))
      , out_(a.size())
      , tmpa2out_(create_backward_plan(tmpa_, out_, planning_flags())) {
    if (a.size() != b.size()) {
      throw std::invalid_argument("size mismatch in time_delay_estimator ctor");
    }
  }

  /// Compute the time-delay estimate between two timeseries
  std::pair<bool, precision_type>
  estimate_delay(timeseries_type& a, timeseries_type& b) {
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
    for (std::size_t i = 0; i != tmpa_.size(); ++i) {
      tmpa_[i] = std::conj(tmpa_[i]) * tmpb_[i];
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
    if (jb::detail::always_aligned<timeseries_t>::value) {
      return FFTW_MEASURE;
    }
    return FFTW_MEASURE | FFTW_UNALIGNED;
  }

private:
  frequency_timeseries_type tmpa_;
  frequency_timeseries_type tmpb_;
  dplan a2tmpa_;
  dplan b2tmpb_;
  output_timeseries_type out_;
  iplan tmpa2out_;
};

} // namespace fftw
} // namespace jb

#endif // jb_fftw_time_delay_estimator_hpp
