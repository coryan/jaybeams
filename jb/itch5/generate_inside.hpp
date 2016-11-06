#ifndef jb_itch5_generate_inside_hpp
#define jb_itch5_generate_inside_hpp

#include <jb/itch5/compute_book.hpp>
#include <jb/offline_feed_statistics.hpp>

#include <iostream>

namespace jb {
namespace itch5 {

/**
 * Determine if this event changes the inside, if so, record the
 * statistics.
 *
 * @tparam duration_t the type used to record the processing latency,
 * must be compatible with a duration in the std::chrono sense.
 *
 * @param stats where to record the statistics
 * @param header (unused) the header for the message that generated
 * this book update
 * @param book the book updated
 * @param update a report of the changes to the book
 * @param processing_latency the time it took to process the event
 * (before the any output is generated).
 * @returns true if the inside is affected by the change, false otherwise.
 */
template <typename duration_t>
bool record_latency_stats(
    jb::offline_feed_statistics& stats,
    jb::itch5::message_header const& header, jb::itch5::order_book const& book,
    jb::itch5::compute_book::book_update const& update,
    duration_t processing_latency) {
  // ... we need to treat each side differently ...
  if (update.buy_sell_indicator == u'B') {
    // ... if the update price is strictly less than the current best
    // bid, that means it did not change the inside.  Notice that this
    // works even when there is no best bid, because the book returns
    // "0" as a best bid in that case ...
    if (update.px < book.best_bid().first) {
      return false;
    }
  } else {
    // ... do the analogous thing for the sell side ...
    if (update.px > book.best_offer().first) {
      return false;
    }
  }
  stats.sample(header.timestamp.ts, processing_latency);
  return true;
}

/**
 * Determine if this event changes the inside, if so, record the
 * statistics and output the result.
 *
 * @tparam duration_t the type used to record the processing latency,
 * must be compatible with a duration in the std::chrono sense.
 *
 * @param stats where to record the statistics
 * @param out where to send the new inside quote if needed
 * @param header (unused) the header for the message that generated
 * this book update
 * @param book the book updated
 * @param update a report of the changes to the book
 * @param processing_latency the time it took to process the event
 * (before the any output is generated).
 * @returns true if the inside is affected by the change, false otherwise.
 */
template <typename duration_t>
bool generate_inside(
    jb::offline_feed_statistics& stats, std::ostream& out,
    jb::itch5::message_header const& header, jb::itch5::order_book const& book,
    jb::itch5::compute_book::book_update const& update,
    duration_t processing_latency) {
  if (not record_latency_stats(
          stats, header, book, update, processing_latency)) {
    return false;
  }
  auto bid = book.best_bid();
  auto offer = book.best_offer();
  out << header.timestamp.ts.count() << " " << header.stock_locate << " "
      << update.stock << " " << bid.first.as_integer() << " " << bid.second
      << " " << offer.first.as_integer() << " " << offer.second << "\n";
  return true;
}

} // namespace itch5
} // namespace jb

#endif // jb_itch5_generate_inside_hpp
