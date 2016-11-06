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
                    << " existing data=" << data << ", msg=" << msg;
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
    add_order_mpid_message const& msg) {
  // ... delegate to the handler for add_order_message ...
  handle_message(
      recvts, msgcnt, msgoffset, static_cast<add_order_message const&>(msg));
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
