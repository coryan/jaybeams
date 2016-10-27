#include <jb/itch5/compute_base.hpp>

/**
 * Verify callback condition, call the callback if successful.
 */
void jb::itch5::compute_base::check_callback_condition(time_point _ts,
							     message_header const& _msg_header,
							     update_result const& _result,
							     bool const _is_inside) {
  auto book_it = std::get<2>(_result);     // ... gets the book iterator
  if (book_it != books_.end()) {           // ... nothing to do if .end(), warning already reported: JB_LOG
      call_callback_condition(
			      _ts,
			      _msg_header,
			      book_it->first,                     // stock
			      book_it->second.best_bid(),
			      book_it->second.best_offer(),
			      book_it->second.get_book_depth(),
			      std::get<0>(_result) or _is_inside);
  }
}

void jb::itch5::compute_base::handle_message(
    time_point recv_ts, long msgcnt, std::size_t msgoffset,
    stock_directory_message const& msg) {
  // Only log these messages if we want super-verbose output, there
  // are nearly 8,200 securities in the Nasdaq exchange.
  JB_LOG(trace) << " " << msgcnt << ":" << msgoffset << " " << msg;
  // ... create the book and update the map ...
  books_.emplace(msg.stock, order_book_depth());
}

void jb::itch5::compute_base::handle_message(
    time_point recv_ts, long msgcnt, std::size_t msgoffset,
    add_order_message const& msg) {
  JB_LOG(trace) << " " << msgcnt << ":" << msgoffset << " " << msg;
  auto result = handle_add_no_update(recv_ts, msgcnt, msgoffset, msg);
  check_callback_condition(recv_ts, msg.header, result);
}

void jb::itch5::compute_base::handle_message(
    time_point recv_ts, long msgcnt, std::size_t msgoffset,
    order_executed_message const& msg) {
  JB_LOG(trace) << " " << msgcnt << ":" << msgoffset << " " << msg;
  handle_reduce(
      recv_ts, msgcnt, msgoffset, msg.header, msg.order_reference_number,
      msg.executed_shares, false);
}

void jb::itch5::compute_base::handle_message(
    time_point recv_ts, long msgcnt, std::size_t msgoffset,
    order_cancel_message const& msg) {
  JB_LOG(trace) << " " << msgcnt << ":" << msgoffset << " " << msg;
  handle_reduce(
      recv_ts, msgcnt, msgoffset, msg.header, msg.order_reference_number,
      msg.canceled_shares, false);
}

void jb::itch5::compute_base::handle_message(
    time_point recv_ts, long msgcnt, std::size_t msgoffset,
    order_delete_message const& msg) {
  JB_LOG(trace) << " " << msgcnt << ":" << msgoffset << " " << msg;
  handle_reduce(
      recv_ts, msgcnt, msgoffset, msg.header, msg.order_reference_number,
      0, true);
}

void jb::itch5::compute_base::handle_message(
    time_point recv_ts, long msgcnt, std::size_t msgoffset,
    order_replace_message const& msg) {
  JB_LOG(trace) << " " << msgcnt << ":" << msgoffset << " " << msg;
  // First we treat the replace as a full cancel, but we do not want
  // to send an update because the operation is supposed to be atomic
  // ...
  auto result_reduce = handle_reduce_no_update(
      recv_ts, msgcnt, msgoffset, msg.header,
      msg.original_order_reference_number, 0, true);
  // ... result_reduce contains a copy of the state of the order before
  // it was removed, use it to create the missing attributes of the
  // new order ...
  if (std::get<2>(result_reduce) == books_.end())  // check if there is a valid book
    return;    // could not find the order to replace, skip the message
  order_data const& copy = std::get<1>(result_reduce);
  // ... handle the replacing order as a new order
  auto result_add = handle_add_no_update(
      recv_ts, msgcnt, msgoffset, add_order_message{
        msg.header, msg.new_order_reference_number,
        copy.buy_sell_indicator, msg.shares, copy.stock, msg.price} );
  check_callback_condition(recv_ts, msg.header, result_add, std::get<0>(result_reduce));
}

void jb::itch5::compute_base::handle_unknown(
    time_point recv_ts, unknown_message const& msg) {
  char msgtype = *static_cast<char const*>(msg.buf());
  JB_LOG(error) << "Unknown message type '" << msgtype << "'("
                << int(msgtype) << ") in msgcnt=" << msg.count()
                << ", msgoffset=" << msg.offset();
}

jb::itch5::compute_base::update_result
jb::itch5::compute_base::handle_add_no_update(
    time_point recv_ts, long msgcnt, std::size_t msgoffset,
    add_order_message const& msg) {
  // First we need to insert the order into the list of active orders ...
  auto position = orders_.emplace(
      msg.order_reference_number, order_data{
        msg.stock, msg.buy_sell_indicator, msg.price, msg.shares});
  if (position.second == false) {
    // ... ooops, this should not happen, there is a problem with the
    // feed, log the problem and skip the message ...
    auto const& data = position.first->second;
    JB_LOG(warning) << "duplicate order id=" << msg.order_reference_number
                    << " existing.symbol=" << data.stock
                    << ", existing.buy_sell_indicator="
                    << data.buy_sell_indicator
                    << ", existing.px=" << data.px
                    << ", existing.qty=" << data.qty
                    << ", msg=" << msg;
    return std::make_tuple(false, order_data(), books_.end());
  }
  // ... okay, now that the order is inserted, let's make sure there
  // is a book for the symbol, we avoid creating a full order book in
  // the normal case ...
  auto stock_book_it = books_.find(msg.stock);
  if (stock_book_it == books_.end()) {
    auto book_pair = books_.emplace(msg.stock, order_book_depth());
    stock_book_it = book_pair.first;                      // iterator to the new order book
  }
  // ... add the order to the book
  bool is_inside = stock_book_it->second.handle_add_order(
      msg.buy_sell_indicator, msg.price, msg.shares);
  return std::make_tuple(is_inside, position.first->second, stock_book_it);
}

void jb::itch5::compute_base::handle_reduce(
    time_point recv_ts, long msgcnt, std::size_t msgoffset,
    message_header const& header, std::uint64_t order_reference_number,
    std::uint32_t shares, bool all_shares) {
  auto result = handle_reduce_no_update(
      recv_ts, msgcnt, msgoffset, header, order_reference_number,
      shares, all_shares);
  check_callback_condition(recv_ts, header, result);
}

jb::itch5::compute_base::update_result
jb::itch5::compute_base::handle_reduce_no_update(
    time_point recv_ts, long msgcnt, std::size_t msgoffset,
    message_header const& header, std::uint64_t order_reference_number,
    std::uint32_t shares, bool all_shares) {
  // First we need to find the order ...
  auto position = orders_.find(order_reference_number);
  if (position == orders_.end()) {
    // ... ooops, this should not happen, there is a problem with the
    // feed, log the problem and skip the message ...
    JB_LOG(warning) << "missing order id=" << order_reference_number
                    << ", location=" << msgcnt << ":" << msgoffset
                    << ", header=" << header;
    return std::make_tuple(false, order_data(), books_.end());
  }
  // ... okay, now that the order is located, find the book for that
  // symbol ...
  auto& data = position->second;
  auto stock_book_it = books_.find(data.stock);
  if (stock_book_it == books_.end()) {
    // ... ooops, this should not happen, there is a problem with the
    // book... an order existed (position) but there is no book for that symbol
    // ...log the problem and skip the message ...
    JB_LOG(warning) << "missing book for symbol id: " << data.stock 
                    << ", order id=" << order_reference_number
                    << ", location=" << msgcnt << ":" << msgoffset
                    << ", header=" << header;
    return std::make_tuple(false, order_data(), books_.end());    // error= books_.end()
  }
  // ... now we need to update the data for the order ...
  if (all_shares) {
    shares = data.qty;
    data.qty = 0;
  }
  data.qty -= shares;
  // ... if the order is finished we need to remove it, otherwise the
  // number of live orders grows without bound (almost), this might
  // remove the data, so we make a copy ...
  order_data copy = data;
  if (data.qty <= 0) {
    // ... if this execution finishes the order we need to remove it
    // from the book ...
    orders_.erase(position);
  }
  // ... finally we can handle the update ...
  bool is_inside = stock_book_it->second.handle_order_reduced(
      copy.buy_sell_indicator, copy.px, shares);
  return std::make_tuple(is_inside, copy, stock_book_it);
}
