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
  orders_.emplace(
      msg.order_reference_number, order_data{
        msg.stock, msg.buy_sell_indicator, msg.price, msg.shares});
  auto i = books_.find(msg.stock);
  if (i == books_.end()) {
    auto p = books_.emplace(msg.stock, order_book());
    i = p.first;
  }
  i->second.handle_add_order(msg.buy_sell_indicator, msg.price, msg.shares);
}

void jb::itch5::compute_inside::handle_unknown(
    time_point recv_ts, long msgcnt, std::size_t msgoffset,
    char const* msgbuf, std::size_t msglen) {
  JB_LOG(error) << "Unknown message type '" << msgbuf[0] << "'"
                << " in msgcnt=" << msgcnt << ", msgoffset=" << msgoffset;
}
