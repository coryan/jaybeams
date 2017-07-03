#ifndef jb_testing_delay_timeseries_hpp
#define jb_testing_delay_timeseries_hpp

#include <jb/detail/array_traits.hpp>

#include <cstddef>
#include <cstdint>
#include <utility>

namespace jb {
namespace testing {

/**
 * A functor to extrapolate with zeroes.
 */
template <typename sample_t>
struct extrapolate_with_zeroes {
  std::pair<std::ptrdiff_t, sample_t>
  operator()(std::intmax_t index, std::size_t size) const {
    if (index < 0 or size <= std::size_t(index)) {
      return std::make_pair(static_cast<std::ptrdiff_t>(-1), sample_t(0));
    }
    return std::make_pair(static_cast<std::ptrdiff_t>(index), sample_t(0));
  }
};

/**
 * A functor to extrapolate a periodic timseries.
 */
template <typename sample_t>
struct extrapolate_periodic {
  std::pair<std::ptrdiff_t, sample_t>
  operator()(std::intmax_t index, std::size_t size) const {
    if (size == 0) {
      return std::make_pair(static_cast<std::ptrdiff_t>(0), sample_t(0));
    }
    if (index < 0) {
      index = size - (-index % size);
    }
    index = index % size;
    return std::make_pair(static_cast<std::ptrdiff_t>(index), sample_t(0));
  }
};

/**
 * A function to get the extrapolated value of a timeseries.
 *
 * @param ts a timeseries, typically a std::vector-like data structure
 * @param t the timestamp at which we want to extrapolate the
 *   timeseries
 * @param sampling_period the sampling period of the timeseries
 * @param extrapolation implement the extrapolation
 *
 * @tparam timeseries_t the type of the timeseries
 * @tparam duration_t the type used to represent timestamps, typically
 *   a std::chrono::duration<> instantiation
 * @tparam extrapolation_functor the type of the extrapolation
 */
template <
    typename timeseries_t, typename duration_t, typename extrapolation_functor>
typename jb::detail::array_traits<timeseries_t>::element_type
extrapolate_timeseries(
    timeseries_t const& ts, duration_t t, duration_t sampling_period,
    extrapolation_functor const& extrapolation) {
  auto ticks = t / sampling_period;
  auto r = extrapolation(ticks, jb::detail::element_count(ts));
  if (r.first == -1) {
    return r.second;
  }
  return *(ts.data() + static_cast<std::size_t>(r.first));
}

/**
 * Delay a timeseries using a user-provided extrapolation policy.
 */
template <
    typename timeseries_t, typename duration_t, typename extrapolation_functor>
timeseries_t delay_timeseries(
    timeseries_t const& ts, duration_t delay, duration_t sampling_period,
    extrapolation_functor const& extrapolation) {
  timeseries_t a(jb::detail::array_shape(ts));

  /// The values stored in the input timeseries ts
  using element_type =
      typename jb::detail::array_traits<timeseries_t>::element_type;

  element_type* it_a = a.data();
  for (std::size_t i = 0; i != jb::detail::element_count(a); ++i, ++it_a) {
    duration_t stamp = i * sampling_period - delay;
    *it_a = extrapolate_timeseries(ts, stamp, sampling_period, extrapolation);
  }
  return std::move(a);
}

/**
 * Delay a timeseries using a periodic extension for early values.
 */
template <typename timeseries_t, typename duration_t>
timeseries_t delay_timeseries_periodic(
    timeseries_t const& ts, duration_t delay, duration_t sampling_period) {
  /// The values stored in the input timeseries ts
  using element_type =
      typename jb::detail::array_traits<timeseries_t>::element_type;
  return delay_timeseries(
      ts, delay, sampling_period, extrapolate_periodic<element_type>());
}

/**
 * Delay a timeseries using zeroes for early values.
 */
template <typename timeseries_t, typename duration_t>
timeseries_t delay_timeseries_zeroes(
    timeseries_t const& ts, duration_t delay, duration_t sampling_period) {
  /// The values stored in the input timeseries ts
  using element_type =
      typename jb::detail::array_traits<timeseries_t>::element_type;
  return delay_timeseries(
      ts, delay, sampling_period, extrapolate_with_zeroes<element_type>());
}

} // namespace testing
} // namespace jb

#endif // jb_testing_delay_timeseries_hpp
