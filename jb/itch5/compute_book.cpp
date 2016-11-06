#include "jb/itch5/compute_book.hpp"

#include <algorithm>

namespace jb {
namespace itch5 {

compute_book::compute_book(callback_type&& cb)
    : callback_(std::forward<callback_type>(cb))
    , books_()
    , orders_() {
}

compute_book::compute_book(callback_type const& cb)
    : compute_book(callback_type(cb)) {
}

void compute_book::handle_message(
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
    JB_LOG(warning) << "duplicate order id=" << msg.order_reference_number
                    << ", location=" << msgcnt << ":" << msgoffset
                    << ", existing data=" << data << ", msg=" << msg;
    return;
  }
  // ... find the right book for this order, create one if necessary ...
  auto iterator = books_.find(msg.stock);
  if (iterator == books_.end()) {
    auto p = books_.emplace(msg.stock, order_book());
    iterator = p.first;
    JB_LOG(info) << "inserted book for unknown security, stock=" << msg.stock;
  }
  (void)iterator->second.handle_add_order(
      msg.buy_sell_indicator, msg.price, msg.shares);

  callback_(
      msg.header, iterator->second,
      book_update{recvts, msg.stock, msg.buy_sell_indicator, msg.price,
                  msg.shares});
}

void compute_book::handle_message(
    time_point recvts, long msgcnt, std::size_t msgoffset,
    order_executed_message const& msg) {
  handle_order_reduction(
      recvts, msgcnt, msgoffset, msg.header, msg.order_reference_number,
      msg.executed_shares);
}

void compute_book::handle_message(
    time_point recvts, long msgcnt, std::size_t msgoffset,
    order_cancel_message const& msg) {
  handle_order_reduction(
      recvts, msgcnt, msgoffset, msg.header, msg.order_reference_number,
      msg.canceled_shares);
}

void compute_book::handle_message(
    time_point recvts, long msgcnt, std::size_t msgoffset,
    order_delete_message const& msg) {
  handle_order_reduction(
      recvts, msgcnt, msgoffset, msg.header, msg.order_reference_number, 0);
}

void compute_book::handle_message(
    time_point recvts, long msgcnt, std::size_t msgoffset,
    stock_directory_message const& msg) {
  JB_LOG(trace) << " " << msgcnt << ":" << msgoffset << " " << msg;
  // ... create the book and update the map ...
  books_.emplace(msg.stock, order_book());
}

std::vector<stock_t> compute_book::symbols() const {
  std::vector<stock_t> result(books_.size());
  std::transform(
      books_.begin(), books_.end(), result.begin(),
      [](auto const& x) { return x.first; });
  return result;
}

void compute_book::handle_order_reduction(
    time_point recvts, long msgcnt, std::size_t msgoffset,
    message_header const& header, std::uint64_t order_reference_number,
    std::uint32_t shares) {
  // First we need to find the order ...
  auto position = orders_.find(order_reference_number);
  if (position == orders_.end()) {
    // ... ooops, this should not happen, there is a problem with the
    // feed, log the problem and skip the message ...
    JB_LOG(warning) << "duplicate order id=" << order_reference_number
                    << ", location=" << msgcnt << ":" << msgoffset
                    << ", header=" << header
                    << ", order_reference_number=" << order_reference_number
                    << ", shares=" << shares;
    return;
  }
  auto& data = position->second;
  auto qty = shares;
  // ... now we need to update the data for the order ...
  if (qty == 0 or data.qty < qty) {
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
  auto& book = books_[u.stock];
  (void)book.handle_order_reduced(u.buy_sell_indicator, u.px, qty);
  callback_(header, book, u);
}

std::ostream& operator<<(std::ostream& os, compute_book::book_update const& x) {
  return os << "{" << x.stock << "," << x.buy_sell_indicator << "," << x.px
            << "," << x.qty << "}";
}

std::ostream& operator<<(std::ostream& os, compute_book::order_data const& x) {
  return os << "{" << x.stock << "," << x.buy_sell_indicator << "," << x.px
            << "," << x.qty << "}";
}

} // namespace itch5
} // namespace jb
