#ifndef jb_itch5_compute_book_depth_hpp
#define jb_itch5_compute_book_depth_hpp

#include <jb/itch5/order_book_depth.hpp>
#include <jb/itch5/add_order_message.hpp>
#include <jb/itch5/add_order_mpid_message.hpp>
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

/* Ticket https://github.com/coryan/jaybeams/issues/20
 * 
 * Uses order_book_depth to get the book depth
 * Change the callback signature to report it
 * Calls the callback on any event (changes to the book)
 *
 */

/**
 * An implementation of jb::message_handler_concept to compute the inside.
 *
 * Keep a collection of all the order books, and forward the right
 * updates to them as it handles the different message types in
 * ITCH-5.0.
 */
class compute_book_depth {
 public:
  //@{
  /**
   * @name Type traits
   */
  /// Define the clock used to measure processing delays
  typedef std::chrono::steady_clock::time_point time_point;

 /* Ticket #20
  * Callback continues returning
  *     time_point, _header, stock 
  * but does not return the BBO (half_quote) any longer
  * returns the book_depth instead
  */
  typedef std::function<void(
      time_point, message_header const&, stock_t const&,
      book_depth_t const&)> callback_type;
  
  /// Initialize an empty handler
  compute_book_depth(callback_type const& callback);

  /// Return the current timestamp for delay measurements
  time_point now() const {
    return std::chrono::steady_clock::now();
  }

  /**
   * Pre-populate the books based on the symbol directory.
   *
   * ITCH-5.0 sends the list of expected securities to be traded on a
   * given day as a sequence of messages.  We use these messages to
   * pre-populate the map of books and avoid hash map updates during
   * the critical path.
   */
  void handle_message(
      time_point recv_ts, long msgcnt, std::size_t msgoffset,
      stock_directory_message const& msg);

  /**
   * Handle a new order.
   */
  void handle_message(
      time_point recv_ts, long msgcnt, std::size_t msgoffset,
      add_order_message const& msg);

  /**
   * Handle a new order with MPID.
   */
  void handle_message(
      time_point recv_ts, long msgcnt, std::size_t msgoffset,
      add_order_mpid_message const& msg) {
    // Delegate on the handler for add_order_message
    handle_message(
        recv_ts, msgcnt, msgoffset,
        static_cast<add_order_message const&>(msg));
  }

  /**
   * Handle an order execution.
   */
  void handle_message(
      time_point recv_ts, long msgcnt, std::size_t msgoffset,
      order_executed_message const& msg);

  /**
   * Handle an order execution.
   */
  void handle_message(
      time_point recv_ts, long msgcnt, std::size_t msgoffset,
      order_executed_price_message const& msg) {
    // Delegate on the handler for add_order_message
    handle_message(
        recv_ts, msgcnt, msgoffset,
        static_cast<order_executed_message const&>(msg));
  }
  
  /**
   * Handle a partial cancel.
   */
  void handle_message(
      time_point recv_ts, long msgcnt, std::size_t msgoffset,
      order_cancel_message const& msg);

  /**
   * Handle a full cancel.
   */
  void handle_message(
      time_point recv_ts, long msgcnt, std::size_t msgoffset,
      order_delete_message const& msg);

  /**
   * Handle an order replace.
   */
  void handle_message(
      time_point recv_ts, long msgcnt, std::size_t msgoffset,
      order_replace_message const& msg);

  /**
   * Ignore all other message types.
   *
   * We are only interested in a handful of message types, anything
   * else is captured by this template function and ignored.
   */
  template<typename message_type>
  void handle_message(
      time_point recv_ts, long msgcnt, std::size_t msgoffset,
      message_type const& msg)
  {}

  /**
   * Log any unknown message types.
   */
  void handle_unknown(time_point recv_ts, unknown_message const& msg);

  /**
   * A convenient container for per-order data.
   *
   * Most market data feeds resend the security identifier and side
   * with each order update, but ITCH-5.0 does not.  One needs to
   * lookup that information based on the order id.  This literal type
   * is used to keep that information around.
   */
  struct order_data {
    stock_t stock;
    buy_sell_indicator_t buy_sell_indicator;
    price4_t px;
    int qty;
  };

  /// An accessor to make testing easier
  std::size_t live_order_count() const {
    return orders_.size();
  }

 private:
  /// The list of live orders
  typedef std::unordered_map<std::uint64_t, order_data> orders_by_id;

  /// The collection of order book depths
  typedef std::unordered_map<
    stock_t, order_book_depth, boost::hash<stock_t>> books_by_security;

  /// The result of a reduction is fairly complex ...
  typedef std::tuple<bool, order_data, books_by_security::iterator> update_result;

  /// Refactor handling of add_order_message for both add_order and
  /// replace.
  update_result handle_add_order(
      time_point recv_ts, long msgcnt, std::size_t msgoffset,
      add_order_message const& msg);

  /// Handle both order executions and partial cancels
  void handle_reduce(
      time_point recv_ts, long msgcnt, std::size_t msgoffset,
      message_header const& header, std::uint64_t order_reference_number,
      std::uint32_t shares, bool all_shares);

  /// Handle an order reduction, but do not update the callback
  update_result handle_reduce_no_update(
      time_point recv_ts, long msgcnt, std::size_t msgoffset,
      message_header const& header, std::uint64_t order_reference_number,
      std::uint32_t shares, bool all_shares);

 private:
  /// Store the callback ...
  callback_type callback_;

  orders_by_id orders_;

  books_by_security books_;
};

} // namespace itch5
} // namespace jb

#endif // jb_itch5_compute_book_depth_hpp
