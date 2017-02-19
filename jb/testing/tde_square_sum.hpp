#ifndef jb_testing_tde_square_sum_hpp
#define jb_testing_tde_square_sum_hpp

#include <jb/fftw/tde_result.hpp>

namespace jb {
namespace testing {

/**
 * Compute the sum square of a family of timeseries.
 *
 * @return tde_result with the square sum of each timeseries
 *
 * @tparam container_t type of the collection of timeseries
 */
template <typename container_t>
auto sum_square(container_t const& ts) {
  using array_type = container_t;
  using element_type =
      typename jb::detail::array_traits<array_type>::element_type;
  /// Extract T out of std::complex<T>, otherwise simply T.
  using precision_type =
      typename jb::extract_value_type<element_type>::precision;
  using sum2_type = jb::fftw::tde_result<array_type, precision_type>;

  sum2_type sum2(ts);
  std::size_t nsamples = jb::detail::nsamples(ts);
  std::size_t num_timeseries = jb::detail::element_count(ts) / nsamples;

  element_type const* it_ts = ts.data();
  for (std::size_t i = 0; i != num_timeseries; ++i) {
    element_type sum2_val = element_type();
    for (std::size_t j = 0; j != nsamples; ++j, ++it_ts) {
      sum2_val += *it_ts * (*it_ts);
    }
    sum2[i] = std::abs(sum2_val);
  }
  return std::move(sum2);
}

} // namespace testing
} // namespace jb

#endif // jb_testing_tde_square_sum_hpp
