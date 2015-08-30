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
 */
template<typename sample_t, typename duration_t>
class timeseries : private std::vector<sample_t> {
 public:
  //@{
  /**
   * @name Type traits
   */
  typedef duration_t duration_type;
  typedef std::vector<sample_t> container;
  using typename container::iterator;
  using typename container::const_iterator;
  using typename container::value_type;
  //@}

  timeseries(
      duration_type sampling_rate,
      duration_type initial_timestamp = duration_type(0))
      : timeseries(sampling_rate, initial_timestamp, {})
  {}

  timeseries(
      duration_type sampling_rate, duration_type initial_timestamp,
      std::initializer_list<value_type> const& list)
      : std::vector<sample_t>(list)
      , sampling_rate_(sampling_rate)
      , initial_timestamp_(initial_timestamp)
  {}

  timeseries(
      duration_type sampling_rate, duration_type initial_timestamp,
      std::initializer_list<value_type>&& list)
      : std::vector<sample_t>(
          std::forward<std::initializer_list<value_type>>(list))
      , sampling_rate_(sampling_rate)
      , initial_timestamp_(initial_timestamp)
  {}

  template<typename... T>
  timeseries(
      duration_type sampling_rate, duration_type initial_timestamp, T&&... t)
      : std::vector<sample_t>(std::forward<T>(t)...)
      , sampling_rate_(sampling_rate)
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

  duration_type sampling_period_start(std::size_t i) const {
    return initial_timestamp_ + i * sampling_rate_;
  }
  duration_type sampling_period_end(std::size_t i) const {
    return sampling_period_end(i + 1);
  }
  duration_type sampling_period_start(const_iterator i) const {
    return initial_timestamp_ + sampling_rate_ * std::distance(begin(), i);
  }
  duration_type sampling_period_end(const_iterator i) const {
    return sampling_period_end(i + 1);
  }
  duration_type sampling_period_start(iterator i) const {
    return initial_timestamp_ + sampling_rate_ * std::distance(begin(), i);
  }
  duration_type sampling_period_end(iterator i) const {
    return sampling_period_end(i + 1);
  }

  typedef std::vector<duration_type> timestamp_list;
  timestamp_list timestamps() const {
    timestamp_list r;
    for (const_iterator i = begin(); i != end(); ++i) {
      r.push_back(sampling_period_start(i));
    }
    return std::move(r);
  }

 private:
  duration_t sampling_rate_;
  duration_t initial_timestamp_;
};

} // namespace jb

#endif // jb_timeseries_hpp
