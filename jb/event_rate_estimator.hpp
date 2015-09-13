#ifndef jb_event_rate_estimator_hpp
#define jb_event_rate_estimator_hpp

#include <chrono>
#include <cstdint>
#include <limits>
#include <sstream>
#include <stdexcept>
#include <type_traits>
#include <vector>

namespace jb {

/**
 * Estimate event rates over a trailing measurement period.
 *
 * Assume you are interested in statistics about events per
 * millisecond.  This class estimates the number of events per
 * trailing millisecond, using an arbitrary sampling period.  For
 * example, you could set the sampling period to 1 microsecond, this
 * class would then estimate the event rate over the previous
 * millisecond for every microsecond.  Or you could set the sampling
 * period to 1 millisecond, which would then estimate the event rate
 * on each millisecond.  Admittedly, for most practical purposes
 * measuring event rates over an exact millisecond boundary or over
 * a trailing millisecond has limited interest, but it was fun to
 * write this class.
 *
 * Given a @a measurement_period: the time over which we want to
 * measure event rates, and @a sampling_period: how often we want to
 * measure the event rates, the class keeps a circular buffer of N
 * counters, representing the trailing sampling periods.  N is chosen
 * such that
 *   @f$ N > {{measurement\_period} \over {sampling\_period}} @f$.
 *
 * As new events arrive the counter for the current sampling period is
 * incremented, once an event in a new sampling period is observed the
 * class emits updates (via a functor) to update the estimated event
 * rate.
 *
 * @tparam duration_t Defines class used to measure time, this
 *   class must be compatible with std::chrono::duration_type.  The
 *   class assumes that event timestamps are measured as durations
 *   with respect to some well-defined (by convention) epoch.  For
 *   example, some data feeds use timestamps as nanoseconds since
 *   midnight, while others use timestamps as microseconds since the
 *   Unix Epoch.
 * @tparam counter_t The type of the counters, most of the time a
 *   simple integer would work well, but if you need to create a large
 *   number of instances of this class, and the values are expected to
 *   be very small, you might consider using a 16-bit or even and
 *   8-bit counter.  Likewise, if you expect very large values for the
 *   counters, such as when measuring event rate per minute, you
 *   should consider using a 64-bit counter.
 */
template<
  typename duration_t = std::chrono::microseconds,
  typename counter_t = int>
class event_rate_estimator {
 public:
  //@{
  /**
   * @name Type traits.
   */
  typedef duration_t duration_type;
  typedef counter_t counter_type;
  //@}

  /**
   * Build an accumulator to estimate event rates.
   *
   * @param measurement_period over what time period we want to
   *   measure message rates
   * @param sampling_period how often do we want to measure message
   *   rates.  Must be smaller than @a measurement_period.
   * @throw std::invalid_argument if the measurement period is not a
   *   multiple of the sampling period.
   */
  event_rate_estimator(
      duration_type measurement_period,
      duration_type sampling_period = duration_type(1))
      : buckets_(bucket_count(measurement_period, sampling_period))
      , sampling_period_(sampling_period)
      , running_total_()
      , last_bucket_()
      , end_pos_(buckets_.size()) {
  }

  /**
   * Record a sample.
   *
   * This class keeps the number of events observed over the last N
   * sampling periods.  New events in the same sampling period are
   * simply recorded, but no rate estimate is made.  When a timestamp
   * in a new sampling period is required to record a sample, the @a
   * update() functor is called to register the new measurements.
   *
   * @param ts the timestamp of the sample.
   * @param update a functor to update when an event rate is estimated.
   */
  template<typename functor>
  void sample(duration_type ts, functor update) {
    if (end_pos_ >= buckets_.size()) {
      // ... this is the first event sample, initialize the circular
      // buffer and just return, there is no rate with a single sample
      // ...
      init(ts);
      return;
    }
    // ... get the bucket number for the timestamps ...
    auto bucket = ts / sampling_period_;
    if (last_bucket_ == bucket) {
      // ... a new sample in the same sampling period, simply
      // increment the counters and continue ...
      running_total_++;
      buckets_[end_pos_]++;
      return;
    }

    // ... yay! new timestamp, rotate the buffer until we get to the
    // new timestamp ...
    while(last_bucket_ < bucket and running_total_ > 0) {
      // ... before rotating, issue an estimate based on the number of
      // samples contained in the circular buffer ...
      update(running_total_, 1);
      rotate();
    }

    // ... we terminate the loop when the running_total_ is 0 because
    // we know we would generate just a lot of updates with 0 value,
    // here we just estimate that number ...
    if (last_bucket_ < bucket) {
      // assert running_total_ == 0
      // for (auto i : buckets_) { assert i == 0; }
      std::uint64_t repeats = bucket - last_bucket_;
      update(0, repeats);
      end_pos_ = 0;
      last_bucket_ = bucket;
    }

    // ... lastly, since this is a new sampling period, record the new
    // event ...
    running_total_++;
    buckets_[end_pos_]++;
  }

  typedef std::vector<counter_type> buckets;

 private:
  /// Just initialize the circular buffer
  void init(duration_type ts) {
    end_pos_ = 0;
    running_total_ = 1;
    buckets_[end_pos_] = 1;
    last_bucket_ = ts / sampling_period_;
  }

  /// Rotate the circular buffer
  void rotate() {
    end_pos_++;
    if (end_pos_ == buckets_.size()) {
      end_pos_ = 0;
    }
    running_total_ -= buckets_[end_pos_];
    buckets_[end_pos_] = 0;
    last_bucket_++;
  }

  /// Estimate the necessary number of buckets
  std::size_t bucket_count(
      duration_type measurement_period, duration_type sampling_period) {
    if (sampling_period <= duration_type(0)) {
      std::ostringstream os;
      os << "jb::event_rate_estimate - sampling period ("
         << sampling_period.count()
         << ") must be a positive number";
      throw std::invalid_argument(os.str());
    }
    if (sampling_period > measurement_period) {
      std::ostringstream os;
      os << "jb::event_rate_estimate - measurement period ("
         << measurement_period.count()
         << ") is smaller than sampling period ("
         << sampling_period.count()
         << ")";
      throw std::invalid_argument(os.str());
    }

    if ((measurement_period % sampling_period).count() != 0) {
      std::ostringstream os;
      os << "jb::event_rate_estimate - measurement period ("
         << measurement_period.count()
         << ") must be a multiple of the sampling period ("
         << sampling_period.count()
         << ")";
      throw std::invalid_argument(os.str());
    }

    // ... because measurement_period and sampling_period are positive
    // numbers N will be a positive number ...
    typedef typename duration_type::rep rep;
    typedef typename std::make_unsigned<rep>::type unsigned_rep;
    unsigned_rep N = measurement_period / sampling_period;
    // ... beware, the type may be larger than what can be stored in
    // an std::size_t (weird segmented memory platforms, yuck) ...
    if (N >= std::numeric_limits<std::size_t>::max()) {
      std::ostringstream os;
      os << "jb::event_rate_estimate - measurement period ("
         << measurement_period.count()
         << ") is too large for sampling period ("
         << sampling_period.count()
         << ")";
      throw std::invalid_argument(os.str());
    }
    return static_cast<std::size_t>(N);
  }

 private:
  /// The time period is bucketized in intervals of 1 sampling period.
  buckets buckets_;

  /// The sampling period
  duration_type sampling_period_;

  /// Current number of events across all buckets.
  std::uint64_t running_total_;

  /// Current number for the sampling period
  typename duration_type::rep last_bucket_;

  /// We treat the buckets as a circular buffer, this is the pointer
  /// to the end of the buffer
  std::size_t end_pos_;
};

} // namespace jb

#endif // jb_event_rate_estimator_hpp
