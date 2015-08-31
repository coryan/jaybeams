#ifndef jb_timeseries_hpp
#define jb_timeseries_hpp

#include <algorithm>
#include <vector>
#include <utility>

namespace jb {

/**
 * Implement a regularly sampled timeseries.
 *
 * JayBeams uses regularly sampled timeseries to represent the signals
 * that are going to be compared against each other for time delay
 * analysis.  We use a fairly simple representation for such a
 * timeseries, just a vector of numbers, typically float, doubles or
 * std::complex<>.  Using a simple representation has many advantages,
 * including (as of C++11) the ability to safely cast the contents of
 * the vector into a simple array, which is what most FFT libraries
 * like.
 *
 * @tparam sample_t the type used to represent the samples, typically
 * float, but can be double, or std::complex<>.  Concievably it could
 * be a struct with a fixed number of numerical values, such as an
 * inside quote.
 * @tparam duration_t the type used to represent the sampling period,
 * typically is one specific instantiation of std::chrono::duration<>
 */
template<typename sample_t, typename duration_t>
class timeseries : private std::vector<sample_t> {
 public:
  //@{
  /**
   * @name Type traits
   */
  typedef duration_t duration_type;
  typedef std::vector<duration_type> timestamp_list;
  typedef std::vector<sample_t> container;
  using typename container::iterator;
  using typename container::const_iterator;
  using typename container::value_type;
  //@}

  /**
   * Constructor
   *
   * @param sampling_period the time between samples.  Notice that
   * this class has no policy to decide how samples that fall in the
   * same bucket are to be merge.
   * @param initial_timestamp the timestamp of the first sample.  In
   * most tests this would be 0, but on production code one would
   * define some sort of Epoch (midnight at the beginning of the day,
   * the Unix Epoch), and make sure all timestamps measurements with
   * respect to that time.
   */
  timeseries(
      duration_type sampling_period,
      duration_type initial_timestamp = duration_type(0))
      : timeseries(sampling_period, initial_timestamp, {})
  {}

  /**
   * Constructor
   *
   * @param sampling_period the time between samples.
   * @param initial_timestamp the timestamp of the first sample.
   * @param list the initial list of values for the timeseries
   *
   * Please see jb::timeseries::timeseries(duration_type,duration_type)
   * for a more detailed description of the arguments.
   */
  timeseries(
      duration_type sampling_period, duration_type initial_timestamp,
      std::initializer_list<value_type> const& list)
      : std::vector<sample_t>(list)
      , sampling_period_(sampling_period)
      , initial_timestamp_(initial_timestamp)
  {}

  /**
   * Constructor
   *
   * @param sampling_period the time between samples.
   * @param initial_timestamp the timestamp of the first sample.
   * @param list the initial list of values for the timeseries
   *
   * Please see jb::timeseries::timeseries(duration_type,duration_type)
   * for a more detailed description of the arguments.
   */
  timeseries(
      duration_type sampling_period, duration_type initial_timestamp,
      std::initializer_list<value_type>&& list)
      : std::vector<sample_t>(
          std::forward<std::initializer_list<value_type>>(list))
      , sampling_period_(sampling_period)
      , initial_timestamp_(initial_timestamp)
  {}

  /**
   * Constructor
   *
   * @param sampling_period the time between samples.
   * @param initial_timestamp the timestamp of the first sample.
   * @param t additional initialization passed directly to the base class.
   *
   * Please see jb::timeseries::timeseries(duration_type,duration_type)
   * for a more detailed description of the arguments.
   */
  template<typename... T>
  timeseries(
      duration_type sampling_period, duration_type initial_timestamp, T&&... t)
      : std::vector<sample_t>(std::forward<T>(t)...)
      , sampling_period_(sampling_period)
      , initial_timestamp_(initial_timestamp)
  {}

  using container::begin;
  using container::end;
  using container::push_back;
  using container::emplace_back;
  using container::at;
  using container::operator[];
  using container::empty;
  using container::size;
  using container::reserve;
  using container::capacity;

  /**
   * Obtain the value of the timeseries at time @a t.
   *
   * This is a convenient way to resample the timeseries at any
   * timestamp.  The extension functor can be used to treat the
   * timeseries as 0 outside the defined range, or extend it
   * periodically, or simply raise an exception.
   *
   * The timeseries is interpolated as-if it was a step function (or
   * "last observation carried forward" in R parlance).
   *
   * @param t the requested timestamp to resample the timeseries at.
   * @param extention a functor to define how the function behaves
   *   when the timestamp is outside the range of the timseries.
   *
   * @tparam extension_functor a functor that takes two parameters, an
   *   (signed) integer and a std::size_t.  The first parameter is the
   *   number of sampling periods between the start of the timeseries
   *   and the requested timestamp @a t.  The second is the size of
   *   the timeseries in sampling periods.
   */
  template<typename extension_functor>
  value_type at(duration_type t, extension_functor extension) const {
    auto ticks = (t - initial_timestamp_) / sampling_period_;
    auto pair = extension(ticks, size());
    if (pair.first == -1) {
      return pair.second;
    }
    return at(pair.first);
  }

  //@{
  /**
   * @name Accessors
   */
  duration_type sampling_period() const {
    return sampling_period_;
  }
  duration_type initial_timestamp() const {
    return initial_timestamp_;
  }
  duration_type sampling_period_start(std::size_t i) const {
    return initial_timestamp_ + i * sampling_period_;
  }
  duration_type sampling_period_end(std::size_t i) const {
    return sampling_period_end(i + 1);
  }
  duration_type sampling_period_start(const_iterator i) const {
    return initial_timestamp_ + sampling_period_ * std::distance(begin(), i);
  }
  duration_type sampling_period_end(const_iterator i) const {
    return sampling_period_end(i + 1);
  }
  duration_type sampling_period_start(iterator i) const {
    return initial_timestamp_ + sampling_period_ * std::distance(begin(), i);
  }
  duration_type sampling_period_end(iterator i) const {
    return sampling_period_end(i + 1);
  }
  //@}

  /**
   * Return the list of timestamps corresponding to this timeseries.
   *
   * The time complexity is $$O(n)$$, this is an expensive operation,
   * use with care.
   */
  timestamp_list timestamps() const {
    timestamp_list r;
    for (const_iterator i = begin(); i != end(); ++i) {
      r.push_back(sampling_period_start(i));
    }
    return std::move(r);
  }

  /**
   * A functor to extend the samples by recycling the values.
   *
   * Use this functor as a parameter to the jb::timeseries::at()
   * function to obtain values as-if the timeseries represented a
   * single period of a periodic function.
   */
  struct extend_by_recycling {
    std::pair<std::ptrdiff_t, sample_t> operator()(
        std::intmax_t index, std::size_t size) const {
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
   * A functor to extend the samples with zeroes.
   *
   * Use this functor as a parameter to the jb::timeseries::at()
   * function to obtain zeroes outside the time range defined by the
   * samples.
   */
  struct extend_by_zeroes {
    std::pair<std::ptrdiff_t, sample_t> operator()(
        std::intmax_t index, std::size_t size) const {
      if (index < 0 or size <= std::size_t(index)) {
        return std::make_pair(static_cast<std::ptrdiff_t>(-1), sample_t(0));
      }
      return std::make_pair(static_cast<std::ptrdiff_t>(index), sample_t(0));
    }
  };

 private:
  duration_t sampling_period_;
  duration_t initial_timestamp_;
};

} // namespace jb

#endif // jb_timeseries_hpp
