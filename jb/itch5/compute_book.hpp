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

/// A convenience alias for clock_type
using clock_type = std::chrono::steady_clock;

/// A convenience alias for clock_type::time_point
using time_point = clock_type::time_point;

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
  time_point recvts;

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

  /// If true, this was a cancel replace and and old order was
  /// modified too...
  bool cxlreplx;

  /// Old price for the order
  price4_t oldpx;

  /// How many shares were removed in the old order
  int oldqty;
};

/**
  * A convenient container for per-order data.
  *
  * Most market data feeds resend the security identifier and side
  * with each order update, but ITCH-5.0 does not.  One needs to
  * lookup the symbol, side, original price,  information based on the order
  * id.  This literal type
  * is used to keep that information around.
  */
struct order_data {
  /// The symbol for this particular order
  stock_t stock;

  /// Whether the order is a BUY or SELL
  buy_sell_indicator_t buy_sell_indicator;

  /// The price of the order
  price4_t px;

  /// The remaining quantity in the order
  int qty;
};

/**
 * Compute the book and call a user-defined callback on each change.
 *
 * Keep a collection of all the order books, indexed by symbol, and
 * forward the updates to them.  Only process the relevant messages
 * types in ITCH-5.0 that are necessary to keep the book.
 *
 * @tparam book_type the type used to define order_book<book_type>,
 * must be compatible with jb::itch5::map_price
 */
template <typename book_type>
class compute_book {
public:
  //@{
  /**
   * @name Type traits
   */
  /// clock_type is used as a compute_book<book_type> type
  /// in some modules
  using clock_type = jb::itch5::clock_type;

  /// time_point is used as a compute_book<book_type> type
  /// in some modules
  using time_point = jb::itch5::time_point;

  /// config type is used to construct the order_book
  using book_type_config = typename book_type::config;

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
  using callback_type = std::function<void(
      message_header const& header, order_book<book_type> const& updated_book,
      book_update const& update)>;
  //@}

  /// Constructor
  explicit compute_book(callback_type&& cb, book_type_config const& cfg)
      : callback_(std::forward<callback_type>(cb))
      , books_()
      , orders_()
      , cfg_(cfg) {
  }

  explicit compute_book(callback_type const& cb, book_type_config const& cfg)
      : compute_book(callback_type(cb), cfg) {
  }

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
      add_order_message const& msg) {
    JB_LOG(trace) << " " << msgcnt << ":" << msgoffset << " " << msg;
    auto insert = orders_.emplace(
        msg.order_reference_number,
        order_data{msg.stock, msg.buy_sell_indicator, msg.price, msg.shares});
    if (insert.second == false) {
      // ... ooops, this should not happen, we got a duplicate order
      // id. There is a problem with the feed, because we are working
      // with simple command-line utilities we are just going to log the
      // error, in a more complex system we would want to raise an
      // exception and let the caller decide what to do ...
      order_data const& data = insert.first->second;
      JB_LOG(warning) << "duplicate order in handle_message(add_order_message)"
                      << ", id=" << msg.order_reference_number
                      << ", location=" << msgcnt << ":" << msgoffset
                      << ", existing data=" << data << ", msg=" << msg;
      return;
    }
    // ... find the right book for this order, create one if necessary ...
    auto itbook = books_.find(msg.stock);
    if (itbook == books_.end()) {
      auto newbk = books_.emplace(msg.stock, order_book<book_type>(cfg_));
      itbook = newbk.first;
    }
    (void)itbook->second.handle_add_order(
        msg.buy_sell_indicator, msg.price, msg.shares);
    callback_(
        msg.header, itbook->second,
        book_update{recvts, msg.stock, msg.buy_sell_indicator, msg.price,
                    msg.shares});
  }

  /**
   * Handle a new order with MPID.
   *
   * @param recvts the timestamp when the message was received
   * @param msgcnt the number of messages received before this message
   * @param msgoffset the number of bytes received before this message
   * @param msg the message describing a new order
   */
  void handle_message(
      time_point recvts, long msgcnt, std::size_t msgoffset,
      add_order_mpid_message const& msg) {
    // ... delegate to the handler for add_order_message (without mpid) ...
    handle_message(
        recvts, msgcnt, msgoffset, static_cast<add_order_message const&>(msg));
  }

  /**
   * Handle an order execution.
   *
   * @param recvts the timestamp when the message was received
   * @param msgcnt the number of messages received before this message
   * @param msgoffset the number of bytes received before this message
   * @param msg the message describing the execution
   */
  void handle_message(
      time_point recvts, long msgcnt, std::size_t msgoffset,
      order_executed_message const& msg) {
    JB_LOG(trace) << " " << msgcnt << ":" << msgoffset << " " << msg;
    handle_order_reduction(
        recvts, msgcnt, msgoffset, msg.header, msg.order_reference_number,
        msg.executed_shares);
  }

  /**
   * Handle an order execution with a different price than the order's
   *
   * @param recvts the timestamp when the message was received
   * @param msgcnt the number of messages received before this message
   * @param msgoffset the number of bytes received before this message
   * @param msg the message describing the execution
   */
  void handle_message(
      time_point recvts, long msgcnt, std::size_t msgoffset,
      order_executed_price_message const& msg) {
    // ... delegate on the handler for add_order_message (without price) ...
    handle_message(
        recvts, msgcnt, msgoffset,
        static_cast<order_executed_message const&>(msg));
  }

  /**
   * Handle a partial cancel.
   *
   * @param recvts the timestamp when the message was received
   * @param msgcnt the number of messages received before this message
   * @param msgoffset the number of bytes received before this message
   * @param msg the message describing the cancelation
   */
  void handle_message(
      time_point recvts, long msgcnt, std::size_t msgoffset,
      order_cancel_message const& msg) {
    JB_LOG(trace) << " " << msgcnt << ":" << msgoffset << " " << msg;
    handle_order_reduction(
        recvts, msgcnt, msgoffset, msg.header, msg.order_reference_number,
        msg.canceled_shares);
  }

  /**
   * Handle a full cancel.
   *
   * @param recvts the timestamp when the message was received
   * @param msgcnt the number of messages received before this message
   * @param msgoffset the number of bytes received before this message
   * @param msg the message describing the cancelation
   */
  void handle_message(
      time_point recvts, long msgcnt, std::size_t msgoffset,
      order_delete_message const& msg) {
    JB_LOG(trace) << " " << msgcnt << ":" << msgoffset << " " << msg;
    handle_order_reduction(
        recvts, msgcnt, msgoffset, msg.header, msg.order_reference_number, 0);
  }

  /**
   * Handle an order replace.
   *
   * @param recvts the timestamp when the message was received
   * @param msgcnt the number of messages received before this message
   * @param msgoffset the number of bytes received before this message
   * @param msg the message describing the cancel/replace
   */
  void handle_message(
      time_point recvts, long msgcnt, std::size_t msgoffset,
      order_replace_message const& msg) {
    JB_LOG(trace) << " " << msgcnt << ":" << msgoffset << " " << msg;
    // First we need to find the original order ...
    auto position = orders_.find(msg.original_order_reference_number);
    if (position == orders_.end()) {
      // ... ooops, this should not happen, there is a problem with the
      // feed, log the problem and skip the message ...
      JB_LOG(warning)
          << "unknown order in handle_message(order_replace_message)"
          << ", id=" << msg.original_order_reference_number
          << ", location=" << msgcnt << ":" << msgoffset << ", msg=" << msg;
      return;
    }
    // ... then we need to make sure the new order is not a duplicate
    // ...
    auto newpos = orders_.find(msg.new_order_reference_number);
    if (newpos != orders_.end()) {
      JB_LOG(warning) << "duplicate order in "
                      << "handle_message(order_replace_message)"
                      << ", id=" << msg.new_order_reference_number
                      << ", location=" << msgcnt << ":" << msgoffset
                      << ", msg=" << msg;
      return;
    }
    // ... find the right book for this order
    // ... the book has to exists, since the original add_order created
    // one if needed
    auto itbook = books_.find(position->second.stock);
    if (itbook != books_.end()) {
      // ... update the order list and book, but do not make a callback ...
      auto update = do_reduce(
          position, itbook->second, recvts, msgcnt, msgoffset, msg.header,
          msg.original_order_reference_number, 0);
      // ... now we need to insert the new order ...
      orders_.emplace(
          msg.new_order_reference_number,
          order_data{update.stock, update.buy_sell_indicator, msg.price,
                     msg.shares});
      (void)itbook->second.handle_add_order(
          update.buy_sell_indicator, msg.price, msg.shares);
      // ... adjust the update data structure ...
      update.cxlreplx = true;
      update.oldpx = update.px;
      update.oldqty = -update.qty;
      update.px = msg.price;
      update.qty = msg.shares;
      // ... and invoke the callback ...
      callback_(msg.header, itbook->second, update);
    }
  }

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
      stock_directory_message const& msg) {
    JB_LOG(trace) << " " << msgcnt << ":" << msgoffset << " " << msg;
    // ... create the book and update the map ...
    books_.emplace(msg.stock, order_book<book_type>(cfg_));
  }

  /**
   * Ignore all other message types.
   *
   * We are only interested in a handful of message types, anything
   * else is captured by this template function and ignored.
   */
  template <typename message_type>
  void handle_message(time_point, long, std::size_t, message_type const&) {
  }

  /**
   * Log any unknown message types.
   *
   * @param recvts the timestamp when the message was received
   * @param msg the unknown message location and contents
   */
  void handle_unknown(time_point recvts, unknown_message const& msg) {
    char msgtype = *static_cast<char const*>(msg.buf());
    JB_LOG(error) << "Unknown message type '" << msgtype << "'(" << int(msgtype)
                  << ") in msgcnt=" << msg.count()
                  << ", msgoffset=" << msg.offset();
  }

  /// Return the symbols known in the order book
  std::vector<stock_t> symbols() const {
    std::vector<stock_t> result(books_.size());
    std::transform(
        books_.begin(), books_.end(), result.begin(),
        [](auto const& x) { return x.first; });
    return result;
  }

  /// Return the current timestamp for delay measurements
  time_point now() const {
    return std::chrono::steady_clock::now();
  }

private:
  /// Represent the collection of order books
  using books_by_security =
      std::unordered_map<stock_t, order_book<book_type>, boost::hash<stock_t>>;

  /// Represent the collection of all orders
  using orders_by_id = std::unordered_map<std::uint64_t, order_data>;
  using orders_iterator = typename orders_by_id::iterator;

  /**
   * Refactor code to handle order reductions, i.e., cancels and
   * executions
   *
   * @param recvts the timestamp when the message was received
   * @param msgcnt the number of messages received before this message
   * @param msgoffset the number of bytes received before this message
   * @param header the header of the message that triggered this event
   * @param order_reference_number the id of the order being reduced
   * @param shares the number of shares to reduce, if 0 reduce all shares
   */
  void handle_order_reduction(
      time_point recvts, long msgcnt, std::size_t msgoffset,
      message_header const& header, std::uint64_t order_reference_number,
      std::uint32_t shares) {
    // First we need to find the order ...
    auto position = orders_.find(order_reference_number);
    if (position == orders_.end()) {
      // ... ooops, this should not happen, there is a problem with the
      // feed, log the problem and skip the message ...
      JB_LOG(warning) << "unknown order in handle_order_reduction"
                      << ", id=" << order_reference_number
                      << ", location=" << msgcnt << ":" << msgoffset
                      << ", header=" << header
                      << ", order_reference_number=" << order_reference_number
                      << ", shares=" << shares;
      return;
    }
    // the book is found, since add_order created it if needed
    auto itbook = books_.find(position->second.stock);
    if (itbook != books_.end()) {
      auto u = do_reduce(
          position, itbook->second, recvts, msgcnt, msgoffset, header,
          order_reference_number, shares);
      callback_(header, itbook->second, u);
    }
  }

  /**
   * Refactor code common to handle_order_reduction() and
   * handle_message(order_replace_message).
   *
   * @param position the location of the order matching order_reference_number
   * @param book the order_book matching the symbol for the given order
   * @param recvts the timestamp when the message was received
   * @param msgcnt the number of messages received before this message
   * @param msgoffset the number of bytes received before this message
   * @param header the header of the message that triggered this event
   * @param order_reference_number the id of the order being reduced
   * @param shares the number of shares to reduce, if 0 reduce all shares
   */
  book_update do_reduce(
      orders_iterator position, order_book<book_type>& book, time_point recvts,
      long msgcnt, std::size_t msgoffset, message_header const& header,
      std::uint64_t order_reference_number, std::uint32_t shares) {
    auto& data = position->second;
    int qty = shares == 0 ? data.qty : static_cast<int>(shares);
    // ... now we need to update the data for the order ...
    if (data.qty < qty) {
      JB_LOG(warning) << "trying to execute more shares than are available"
                      << ", location=" << msgcnt << ":" << msgoffset
                      << ", data=" << data << ", header=" << header
                      << ", order_reference_number=" << order_reference_number
                      << ", shares=" << shares;
      qty = data.qty;
    }
    data.qty -= qty;
    // ... if the order is finished we need to remove it, otherwise the
    // number of live orders grows without bound (almost), this might
    // remove the data, so we make a copy ...
    book_update u{recvts, data.stock, data.buy_sell_indicator, data.px,
                  -static_cast<int>(qty)};
    // ... after the copy is safely stored, go and remove the order if
    // needed ...
    if (data.qty == 0) {
      orders_.erase(position);
    }
    (void)book.handle_order_reduced(u.buy_sell_indicator, u.px, qty);
    return u;
  }

private:
  /// Store the callback function, this is invoked on each event that
  /// changes a book.
  callback_type callback_;

  /// The order books indexed by security.
  books_by_security books_;

  /// The live orders indexed by the "order reference number"
  orders_by_id orders_;

  /// reference to the order book config
  book_type_config const& cfg_;
};

inline bool operator==(book_update const& a, book_update const& b) {
  return a.recvts == b.recvts and a.stock == b.stock and
         a.buy_sell_indicator == b.buy_sell_indicator and a.px == b.px and
         a.qty == b.qty;
}

inline bool operator!=(book_update const& a, book_update const& b) {
  return !(a == b);
}

std::ostream& operator<<(std::ostream& os, book_update const& x) {
  return os << "{" << x.stock << "," << x.buy_sell_indicator << "," << x.px
            << "," << x.qty << "}";
}

std::ostream& operator<<(std::ostream& os, order_data const& x) {
  return os << "{" << x.stock << "," << x.buy_sell_indicator << "," << x.px
            << "," << x.qty << "}";
}

} // namespace itch5
} // namespace jb

#endif // jb_itch5_compute_book_hpp
