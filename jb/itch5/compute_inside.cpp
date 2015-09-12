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
    return;
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
  if (changed_inside) {
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
  // First we need to insert the order into the list of active orders ...
  auto position = orders_.find(msg.order_reference_number);
  if (position == orders_.end()) {
    // ... ooops, this should not happen, there is a problem with the
    // feed, log the problem and skip the message ...
    JB_LOG(warning) << "missing order id=" << msg.order_reference_number
                    << ", location=" << msgcnt << ":" << msgoffset
                    << ", msg=" << msg;
    return;
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
                    << ", msg=" << msg;
    return;
  }

  // ... now we need to update the data for the order, this might
  // remove the data ...
  data.qty -= msg.executed_shares;
  order_data copy = data;
  if (data.qty <= 0) {
    // ... if this execution finishes the order we need to remove it
    // from the book ...
    orders_.erase(position);
  }
  // ... finally we can handle the update ...
  bool changed_inside = i->second.handle_order_reduced(
      copy.buy_sell_indicator, copy.px, msg.executed_shares);
  if (changed_inside) {
    // ... if there is a change at the inside send that to the
    // callback ...
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
