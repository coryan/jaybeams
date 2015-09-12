#include <jb/itch5/compute_inside.hpp>

jb::itch5::compute_inside::compute_inside(callback_type const& cb)
    : callback_(cb)
    , orders_()
    , books_()
{}

void jb::itch5::compute_inside::handle_message(
    time_point recv_ts, long msgcnt, std::size_t msgoffset,
    stock_directory_message const& msg) {
  // Only log these messages if we want super-verbose output, there
  // are nearly 8,200 securities in the Nasdaq exchange.
  JB_LOG(trace) << " " << msgcnt << ":" << msgoffset << " " << msg;
  // ... create the book and update the map ...
  books_.emplace(msg.stock, order_book());
}

void jb::itch5::compute_inside::handle_message(
    time_point recv_ts, long msgcnt, std::size_t msgoffset,
    add_order_message const& msg) {
  JB_LOG(trace) << " " << msgcnt << ":" << msgoffset << " " << msg;
  auto r = handle_add_order(recv_ts, msgcnt, msgoffset, msg);
  if (std::get<0>(r)) {
    auto i = std::get<2>(r);
    // ... if there is a change at the inside send that to the
    // callback ...
    callback_(
        recv_ts, msg.stock, i->second.best_bid(), i->second.best_offer());
  }
}

void jb::itch5::compute_inside::handle_message(
    time_point recv_ts, long msgcnt, std::size_t msgoffset,
    order_executed_message const& msg) {
  JB_LOG(trace) << " " << msgcnt << ":" << msgoffset << " " << msg;
  handle_reduce(
      recv_ts, msgcnt, msgoffset, msg.header, msg.order_reference_number,
      msg.executed_shares, false);
}

void jb::itch5::compute_inside::handle_message(
    time_point recv_ts, long msgcnt, std::size_t msgoffset,
    order_cancel_message const& msg) {
  JB_LOG(trace) << " " << msgcnt << ":" << msgoffset << " " << msg;
  handle_reduce(
      recv_ts, msgcnt, msgoffset, msg.header, msg.order_reference_number,
      msg.canceled_shares, false);
}

void jb::itch5::compute_inside::handle_message(
    time_point recv_ts, long msgcnt, std::size_t msgoffset,
    order_delete_message const& msg) {
  JB_LOG(trace) << " " << msgcnt << ":" << msgoffset << " " << msg;
  handle_reduce(
      recv_ts, msgcnt, msgoffset, msg.header, msg.order_reference_number,
      0, true);
}

void jb::itch5::compute_inside::handle_message(
    time_point recv_ts, long msgcnt, std::size_t msgoffset,
    order_replace_message const& msg) {
  JB_LOG(trace) << " " << msgcnt << ":" << msgoffset << " " << msg;
  // First we treat the replace as a full cancel, but we do not want
  // to send an update because the operation is supposed to be atomic
  // ...
  auto r = handle_reduce_no_update(
      recv_ts, msgcnt, msgoffset, msg.header,
      msg.original_order_reference_number, 0, true);
  // ... the result contains a copy of the state of the order before
  // it was removed, use it to create the missing attributes of the
  // new order ...
  order_data const& copy = std::get<1>(r);
  // ... handle the replacing order as a new order, but delay the
  // decision to update the callback ...
  auto a = handle_add_order(
      recv_ts, msgcnt, msgoffset, add_order_message{
        msg.header, msg.new_order_reference_number,
        copy.buy_sell_indicator, msg.shares, copy.stock, msg.price} );

  // ... finally we can decide if an update is needed ...
  if (std::get<0>(a) or std::get<0>(r)) {
    // ... if there is a change at the inside send that to the
    // callback ...
    auto i = std::get<2>(r);
    callback_(
        recv_ts, copy.stock, i->second.best_bid(), i->second.best_offer());
  }
}

void jb::itch5::compute_inside::handle_unknown(
    time_point recv_ts, long msgcnt, std::size_t msgoffset,
    char const* msgbuf, std::size_t msglen) {
  JB_LOG(error) << "Unknown message type '" << msgbuf[0] << "'"
                << " in msgcnt=" << msgcnt << ", msgoffset=" << msgoffset;
}

jb::itch5::compute_inside::update_result
jb::itch5::compute_inside::handle_add_order(
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
  auto i = books_.find(msg.stock);
  if (i == books_.end()) {
    auto p = books_.emplace(msg.stock, order_book());
    i = p.first;
  }
  // ... add the order to the book and determine if the inside changed
  // ...
  bool changed_inside = i->second.handle_add_order(
      msg.buy_sell_indicator, msg.price, msg.shares);
  return std::make_tuple(changed_inside, position.first->second, i);
}

void jb::itch5::compute_inside::handle_reduce(
    time_point recv_ts, long msgcnt, std::size_t msgoffset,
    message_header const& header, std::uint64_t order_reference_number,
    std::uint32_t shares, bool all_shares) {
  auto r = handle_reduce_no_update(
      recv_ts, msgcnt, msgoffset, header, order_reference_number,
      shares, all_shares);
  if (std::get<0>(r)) {
    auto const& copy = std::get<1>(r);
    auto i = std::get<2>(r);
    // ... if there is a change at the inside send that to the
    // callback ...
    callback_(
        recv_ts, copy.stock, i->second.best_bid(), i->second.best_offer());
  }
}

jb::itch5::compute_inside::update_result
jb::itch5::compute_inside::handle_reduce_no_update(
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
  auto i = books_.find(data.stock);
  if (i == books_.end()) {
    // ... ooops, this should not happen, there is a problem with the
    // code, if the order exists, the book should exist too ...
    JB_LOG(warning) << "missing book for stock=" << data.stock
                    << ", location=" << msgcnt << ":" << msgoffset
                    << ", header=" << header;
    throw std::runtime_error(
        "jb::itch5::compute_inside::handle_reduce_no_update - "
        "internal state corrupted");
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
  bool changed_inside = i->second.handle_order_reduced(
      copy.buy_sell_indicator, copy.px, shares);
  return std::make_tuple(changed_inside, copy, i);
}
