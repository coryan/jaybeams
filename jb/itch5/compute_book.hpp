#ifndef jb_itch5_compute_book_hpp
#define jb_itch5_compute_book_hpp

#include <jb/itch5/add_order_message.hpp>
#include <jb/itch5/add_order_mpid_message.hpp>
#include <jb/itch5/order_book.hpp>
#include <jb/itch5/order_cancel_message.hpp>
#include <jb/itch5/order_delete_message.hpp>
#include <jb/itch5/order_executed_message.hpp>
#include <jb/itch5/order_executed_price_message.hpp>
#include <jb/itch5/order_replace_message.hpp>
#include <jb/itch5/stock_directory_message.hpp>
#include <jb/itch5/unknown_message.hpp>

#include <boost/functional/hash.hpp>
#include <chrono>
#include <functional>
#include <unordered_map>

namespace jb {
namespace itch5 {

/**
 * Compute the book and call a user-defined callback on each change.
 *
 * Keep a collection of all the order books, indexed by symbol, and
 * forward the updates to them.  Only process the relevant messages
 * types in ITCH-5.0 that are necessary to keep the book.
 */
class compute_book {
public:
  //@{
  /**
   * @name Type traits
   */
  /// Define the clock used to measure processing delays
  typedef std::chrono::steady_clock clock_type;

  /// A convenience typedef for clock_type::time_point
  typedef typename clock_type::time_point time_point;

  /**
   * A flat struct to represent updates to an order book.
   *
   * Updates to an order book come in many forms, but they can all be
   * represented with a simple structure that shows: what book is
   * being updated, what side of the book is being updated, what price
   * level is being updated, and how many shares are being added or
   * removed from the book.
   */
  struct book_update {
    /**
     * When was the message that triggered this update received
     */
    time_point recv_ts;

    /**
     * The security updated by this order.  This is redundant for
     * order updates and deletes, and ITCH-5.0 omits the field, but
     * we find it easier
     */
    stock_t stock;

    /// What side of the book is being updated.
    buy_sell_indicator_t buy_sell_indicator;

    /// What price level is being updated.
    price4_t px;

    /// How many shares are being added (if positive) or removed (if
    /// negative) from the book.
    int qty;
  };

  /**
   * Define the callback type
   *
   * A callback of this type is required in the constructor of this
   * class.  After each book update the user-provided callback is
   * invoked.
   *
   * @param header the header of the raw ITCH-5.0 message
   * @param update a representation of the update just applied to the
   * book
   * @param updated_book the order_book after the update was applied
   */
  typedef std::function<void(
      message_header const& header, book_update const& update,
      order_book const& updated_book)>
      callback_type;
  //@}

  /// Constructor
  explicit compute_book(callback_type const& cb);
  explicit compute_book(callback_type&& cb);

  /**
   * Handle a new order message.
   *
   * New orders are added to the list of known orders and their qty is
   * added to the right book at the order's price level.
   *
   * @param recvts the timestamp when the message was received
   * @param msgcnt the number of messages received before this message
   * @param msgoffset the number of bytes received before this message
   * @param msg the message describing a new order
   */
  void handle_message(
      time_point recvts, long msgcnt, std::size_t msgoffset,
      add_order_message const& msg);

  /**
   * Pre-populate the books based on the symbol directory.
   *
   * ITCH-5.0 sends the list of expected securities to be traded on a
   * given day as a sequence of messages.  We use these messages to
   * pre-populate the map of books and avoid hash map updates during
   * the critical path.
   *
   * @param recvts the timestamp when the message was received
   * @param msgcnt the number of messages received before this message
   * @param msgoffset the number of bytes received before this message
   * @param msg the message describing a known symbol for the feed
   */
  void handle_message(
      time_point recvts, long msgcnt, std::size_t msgoffset,
      stock_directory_message const& msg);

  /// Return the symbols known in the order book
  std::vector<stock_t> symbols() const;

  /// Return the current timestamp for delay measurements
  time_point now() const {
    return std::chrono::steady_clock::now();
  }

private:
  /// Represent the collection of order books
  typedef std::unordered_map<stock_t, order_book, boost::hash<stock_t>>
      books_by_security;

private:
  /// Store the callback function, this is invoked on each event that
  /// changes a book.
  callback_type callback_;

  /// The order books indexed by security.
  books_by_security books_;
};

inline bool operator==(
    compute_book::book_update const& a, compute_book::book_update const& b) {
  return a.recv_ts == b.recv_ts and a.stock == b.stock and
         a.buy_sell_indicator == b.buy_sell_indicator and a.px == b.px and
         a.qty == b.qty;
}

inline bool operator!=(
    compute_book::book_update const& a, compute_book::book_update const& b) {
  return !(a == b);
}

std::ostream& operator<<(std::ostream& os, compute_book::book_update const& x);

} // namespace itch5
} // namespace jb

#endif // jb_itch5_compute_book_hpp
