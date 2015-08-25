#ifndef jb_event_rate_estimator_hpp
#define jb_event_rate_estimator_hpp

#include <chrono>
#include <cstdint>
#include <vector>

namespace jb {

/**
 * Helper class to compute statistics about messages frequency.
 *
 * When designing the time delay analysis system it is interesting to
 * answer questions such as these:
 * - How many messages per second arrive on average?  How many per
 * millisecond?
 * - More generally, what is the distribution of messages per second,
 * millisecond and microsecond?
 *
 * This information will define the performance requirements of the
 * system, help us define simulators for the environment, define
 * performance requirements and ultimately measure the performance of
 * the system.
 *
 * We are going to need to measure such statistics per-symbol, there
 * are roughly 15,000 symbols in the US market, and we would like to
 * keep memory utilization for this data to be below 8GiB [1].  That
 * leaves us with roughly 600 KiB per symbol.  So we cannot be just
 * wasteful with memory.
 *
 * Assume you are interested in statistics about messages per
 * millisecond.  In that scenario, this class keeps exactly 1000
 * buckets, representing the last 1000 microseconds or the last
 * rolling millisecond.  Each bucket contains a counter representing
 * the number of events in that microsecond.
 *
 * If a new sample arrives for the current microsecond we simply
 * increment the counter.  If a new sample arrives for the next
 * microsecond we (1) count the number of events in the current
 * buckets, (2) update the statistics, (3) "rotate" the circular
 * buffer by reseting the oldest buffer to 0 and moving the time
 * pointer, and (4) insert the new timestamp in the current bucket. 
 * If the new sample skips one more more microseconds then steps 1-3
 * are repeated until the circular buffer catches up to the new
 * timestamp, and then the counter is incremented.
 *
 * [1] This is an arbitrary limit based on the memory on coryan@
 * workstation (16 GiB).  I decided that only 1/2 of the memory would
 * be dedicated to this application.
 *
 * @tparam duration_type Defines the size of the buckets.
 */
template<typename duration_type = std::chrono::microseconds>
class event_rate_estimator {
 public:
  /**
   * Build an accumulator to measure events-per-period.
   */
  event_rate_estimator(duration_type period)
      : buckets_(period.count())
      , running_total_()
      , last_event_()
      , end_pos_(period.count()) {
  }

  template<typename functor>
  void sample(
      duration_type ts, functor update) {
    if (end_pos_ >= buckets_.size()) {
      init(ts);
      return;
    }
    if (last_event_ == ts) {
      running_total_++;
      buckets_[end_pos_]++;
      return;
    }
    while(last_event_ < ts and running_total_ > 0) {
      update(running_total_, 1);
      rotate();
    }
    if (last_event_ < ts) {
      std::uint64_t repeats = (ts - last_event_).count();
      update(0, repeats);
      end_pos_ += repeats;
      end_pos_ %= buckets_.size();
      last_event_ = ts;
    }
    running_total_++;
    buckets_[end_pos_]++;
  }

  typedef std::vector<std::uint16_t> buckets;

 private:
  void init(duration_type ts) {
    end_pos_ = 0;
    running_total_++;
    buckets_[end_pos_] = 1;
    last_event_ = ts;
  }

  void rotate() {
    end_pos_++;
    if (end_pos_ == buckets_.size()) {
      end_pos_ = 0;
    }
    running_total_ -= buckets_[end_pos_];
    buckets_[end_pos_] = 0;
    last_event_++;
  }

 private:
  /// The time period is bucketized in intervals of 1 microsecond.
  buckets buckets_;

  /// Current number of events across all buckets.
  std::uint64_t running_total_;

  /// Current end time of the period
  duration_type last_event_;

  /// We treat the buckets as a circular buffer, this is the pointer
  /// to the end of the buffer
  std::size_t end_pos_;
};

} // namespace jb

#endif // jb_event_rate_estimator_hpp
