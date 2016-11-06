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
    JB_LOG(warning) << "duplicate order in handle_message(add_order_message)"
                    << ", id=" << msg.order_reference_number
                    << ", location=" << msgcnt << ":" << msgoffset
                    << ", existing data=" << data << ", msg=" << msg;
    return;
  }
  // ... find the right book for this order, create one if necessary ...
  auto& book = books_[msg.stock];
  (void)book.handle_add_order(msg.buy_sell_indicator, msg.price, msg.shares);
  callback_(
      msg.header, book, book_update{recvts, msg.stock, msg.buy_sell_indicator,
                                    msg.price, msg.shares});
}

void compute_book::handle_message(
    time_point recvts, long msgcnt, std::size_t msgoffset,
    order_executed_message const& msg) {
  JB_LOG(trace) << " " << msgcnt << ":" << msgoffset << " " << msg;
  handle_order_reduction(
      recvts, msgcnt, msgoffset, msg.header, msg.order_reference_number,
      msg.executed_shares);
}

void compute_book::handle_message(
    time_point recvts, long msgcnt, std::size_t msgoffset,
    order_cancel_message const& msg) {
  JB_LOG(trace) << " " << msgcnt << ":" << msgoffset << " " << msg;
  handle_order_reduction(
      recvts, msgcnt, msgoffset, msg.header, msg.order_reference_number,
      msg.canceled_shares);
}

void compute_book::handle_message(
    time_point recvts, long msgcnt, std::size_t msgoffset,
    order_delete_message const& msg) {
  JB_LOG(trace) << " " << msgcnt << ":" << msgoffset << " " << msg;
  handle_order_reduction(
      recvts, msgcnt, msgoffset, msg.header, msg.order_reference_number, 0);
}

void compute_book::handle_message(
    time_point recvts, long msgcnt, std::size_t msgoffset,
    order_replace_message const& msg) {
  JB_LOG(trace) << " " << msgcnt << ":" << msgoffset << " " << msg;
  // First we need to find the original order ...
  auto position = orders_.find(msg.original_order_reference_number);
  if (position == orders_.end()) {
    // ... ooops, this should not happen, there is a problem with the
    // feed, log the problem and skip the message ...
    JB_LOG(warning) << "unknown order in handle_message(order_replace_message)"
                    << ", id=" << msg.original_order_reference_number
                    << ", location=" << msgcnt << ":" << msgoffset
                    << ", msg=" << msg;
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
  auto& book = books_[position->second.stock];
  // ... update the order list and book, but do not make a callback ...
  auto update = do_reduce(
      position, book, recvts, msgcnt, msgoffset, msg.header,
      msg.original_order_reference_number, 0);
  // ... now we need to insert the new order ...
  orders_.emplace(
      msg.new_order_reference_number,
      order_data{update.stock, update.buy_sell_indicator, msg.price,
                 msg.shares});
  (void)book.handle_add_order(update.buy_sell_indicator, msg.price, msg.shares);
  // ... adjust the update data structure ...
  update.cxlreplx = true;
  update.oldpx = update.px;
  update.oldqty = -update.qty;
  update.px = msg.price;
  update.qty = msg.shares;
  // ... and invoke the callback ...
  callback_(msg.header, book, update);
}

void compute_book::handle_message(
    time_point recvts, long msgcnt, std::size_t msgoffset,
    stock_directory_message const& msg) {
  JB_LOG(trace) << " " << msgcnt << ":" << msgoffset << " " << msg;
  // ... create the book and update the map ...
  books_.emplace(msg.stock, order_book());
}

void compute_book::handle_unknown(
    time_point recvts, unknown_message const& msg) {
  char msgtype = *static_cast<char const*>(msg.buf());
  JB_LOG(error) << "Unknown message type '" << msgtype << "'(" << int(msgtype)
                << ") in msgcnt=" << msg.count()
                << ", msgoffset=" << msg.offset();
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
    JB_LOG(warning) << "unknown order in handle_order_reduction"
                    << ", id=" << order_reference_number
                    << ", location=" << msgcnt << ":" << msgoffset
                    << ", header=" << header
                    << ", order_reference_number=" << order_reference_number
                    << ", shares=" << shares;
    return;
  }
  auto& book = books_[position->second.stock];
  auto u = do_reduce(
      position, book, recvts, msgcnt, msgoffset, header, order_reference_number,
      shares);
  callback_(header, book, u);
}

compute_book::book_update compute_book::do_reduce(
    orders_by_id::iterator position, order_book& book, time_point recvts,
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
