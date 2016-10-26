#include <jb/itch5/compute_book_depth.hpp>
#include <jb/assert_throw.hpp>

jb::itch5::compute_book_depth::compute_book_depth(callback_type const& cb)
    : callback_(cb)
    , orders_()
    , books_()
{}

void jb::itch5::compute_book_depth::handle_message(
    time_point recv_ts, long msgcnt, std::size_t msgoffset,
    stock_directory_message const& msg) {
  // Only log these messages if we want super-verbose output, there
  // are nearly 8,200 securities in the Nasdaq exchange.
  JB_LOG(trace) << " " << msgcnt << ":" << msgoffset << " " << msg;
  // ... create the book and update the map ...
  books_.emplace(msg.stock, order_book_depth());
}

void jb::itch5::compute_book_depth::handle_message(
    time_point recv_ts, long msgcnt, std::size_t msgoffset,
    add_order_message const& msg) {
  JB_LOG(trace) << " " << msgcnt << ":" << msgoffset << " " << msg;
  auto result = handle_add_order(recv_ts, msgcnt, msgoffset, msg);    
  if (std::get<0>(result)) {     // check if handler reports event = true
    auto book_it = std::get<2>(result);     // ... if so, gets the book iterator
    callback_(
        recv_ts, msg.header, msg.stock,
        book_it->second.get_book_depth());
  }
}

void jb::itch5::compute_book_depth::handle_message(
    time_point recv_ts, long msgcnt, std::size_t msgoffset,
    order_executed_message const& msg) {
  JB_LOG(trace) << " " << msgcnt << ":" << msgoffset << " " << msg;
  handle_reduce(
      recv_ts, msgcnt, msgoffset, msg.header, msg.order_reference_number,
      msg.executed_shares, false);
}

void jb::itch5::compute_book_depth::handle_message(
    time_point recv_ts, long msgcnt, std::size_t msgoffset,
    order_cancel_message const& msg) {
  JB_LOG(trace) << " " << msgcnt << ":" << msgoffset << " " << msg;
  handle_reduce(
      recv_ts, msgcnt, msgoffset, msg.header, msg.order_reference_number,
      msg.canceled_shares, false);
}

void jb::itch5::compute_book_depth::handle_message(
    time_point recv_ts, long msgcnt, std::size_t msgoffset,
    order_delete_message const& msg) {
  JB_LOG(trace) << " " << msgcnt << ":" << msgoffset << " " << msg;
  handle_reduce(
      recv_ts, msgcnt, msgoffset, msg.header, msg.order_reference_number,
      0, true);
}

void jb::itch5::compute_book_depth::handle_message(
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
  order_data const& copy = std::get<1>(result_reduce);
  // ... handle the replacing order as a new order, but delay the
  // decision to update the callback ...
  auto result_add = handle_add_order(
      recv_ts, msgcnt, msgoffset, add_order_message{
        msg.header, msg.new_order_reference_number,
        copy.buy_sell_indicator, msg.shares, copy.stock, msg.price} );

  // ... finally we can decide if an update is needed ...
  if (std::get<0>(result_add) or std::get<0>(result_reduce)) {
    // ... if there is an event send that to the callback ...
    auto book_it = std::get<2>(result_reduce);      // does not matter which result, same book
    callback_(
        recv_ts, msg.header, copy.stock,
        book_it->second.get_book_depth());
  }
}

void jb::itch5::compute_book_depth::handle_unknown(
    time_point recv_ts, unknown_message const& msg) {
  char msgtype = *static_cast<char const*>(msg.buf());
  JB_LOG(error) << "Unknown message type '" << msgtype << "'("
                << int(msgtype) << ") in msgcnt=" << msg.count()
                << ", msgoffset=" << msg.offset();
}

jb::itch5::compute_book_depth::update_result
jb::itch5::compute_book_depth::handle_add_order(
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
  bool is_event = stock_book_it->second.handle_add_order(
      msg.buy_sell_indicator, msg.price, msg.shares);
  return std::make_tuple(is_event, position.first->second, stock_book_it);
}

void jb::itch5::compute_book_depth::handle_reduce(
    time_point recv_ts, long msgcnt, std::size_t msgoffset,
    message_header const& header, std::uint64_t order_reference_number,
    std::uint32_t shares, bool all_shares) {
  auto result = handle_reduce_no_update(
      recv_ts, msgcnt, msgoffset, header, order_reference_number,
      shares, all_shares);
  if (std::get<0>(result)) {
    auto const& copy = std::get<1>(result);
    auto stock_book_it = std::get<2>(result);
    // ... if there is an event send that to the callback ...
    callback_(
        recv_ts, header, copy.stock,
        stock_book_it->second.get_book_depth());
  }
}

jb::itch5::compute_book_depth::update_result
jb::itch5::compute_book_depth::handle_reduce_no_update(
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
  JB_ASSERT_THROW(stock_book_it != books_.end());

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
  bool is_event = stock_book_it->second.handle_order_reduced(
      copy.buy_sell_indicator, copy.px, shares);
  return std::make_tuple(is_event, copy, stock_book_it);
}
