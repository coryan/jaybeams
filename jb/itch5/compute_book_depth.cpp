#include <jb/itch5/compute_book_depth.hpp>
#include <jb/assert_throw.hpp>
#include <jb/feed_error.hpp>

#include <sstream>

jb::itch5::compute_book_depth::compute_book_depth(callback_type const& cb)
    : callback_(cb)
    , orders_()
    , books_() {
}

void jb::itch5::compute_book_depth::handle_message(
    time_point recv_ts, long msgcnt, std::size_t msgoffset,
    stock_directory_message const& msg) {
  // Only log these messages if we want super-verbose output, there
  // are nearly 8,200 securities in the Nasdaq exchange.
  JB_LOG(trace) << " " << msgcnt << ":" << msgoffset << " " << msg;
  // ... create the book and update the map ...
  books_.emplace(msg.stock, order_book());
}

void jb::itch5::compute_book_depth::handle_message(
    time_point recv_ts, long msgcnt, std::size_t msgoffset,
    add_order_message const& msg) {
  JB_LOG(trace) << " " << msgcnt << ":" << msgoffset << " " << msg;
  try {
    auto result = handle_add_no_update(recv_ts, msgcnt, msgoffset, msg);
    auto book_it = std::get<2>(result); // ...gets the book iterator
    callback_(recv_ts, msg.header, msg.stock, book_it->second.get_book_depth());
  } catch (const jb::feed_error& fe) {
    JB_LOG(warning) << "add_order_message skipping the message: " << fe.what();
  }
}

void jb::itch5::compute_book_depth::handle_message(
    time_point recv_ts, long msgcnt, std::size_t msgoffset,
    order_executed_message const& msg) {
  JB_LOG(trace) << " " << msgcnt << ":" << msgoffset << " " << msg;
  handle_reduce(recv_ts, msgcnt, msgoffset, msg.header,
                msg.order_reference_number, msg.executed_shares, false);
}

void jb::itch5::compute_book_depth::handle_message(
    time_point recv_ts, long msgcnt, std::size_t msgoffset,
    order_cancel_message const& msg) {
  JB_LOG(trace) << " " << msgcnt << ":" << msgoffset << " " << msg;
  handle_reduce(recv_ts, msgcnt, msgoffset, msg.header,
                msg.order_reference_number, msg.canceled_shares, false);
}

void jb::itch5::compute_book_depth::handle_message(
    time_point recv_ts, long msgcnt, std::size_t msgoffset,
    order_delete_message const& msg) {
  JB_LOG(trace) << " " << msgcnt << ":" << msgoffset << " " << msg;
  handle_reduce(recv_ts, msgcnt, msgoffset, msg.header,
                msg.order_reference_number, 0, true);
}

void jb::itch5::compute_book_depth::handle_message(
    time_point recv_ts, long msgcnt, std::size_t msgoffset,
    order_replace_message const& msg) {
  JB_LOG(trace) << " " << msgcnt << ":" << msgoffset << " " << msg;
  // First we treat the replace as a full cancel, but we do not want
  // to send an update because the operation is supposed to be atomic...
  try {
    auto result_reduce =
        handle_reduce_no_update(recv_ts, msgcnt, msgoffset, msg.header,
                                msg.original_order_reference_number, 0, true);
    // ... result_reduce contains a copy of the state of the order before
    // it was removed, use it to create the missing attributes of the
    // new order ...
    order_data const& copy = std::get<1>(result_reduce);
    // ... handle the replacing order as a new order, but delay the
    // decision to update the callback ...
    auto result_add = handle_add_no_update(
        recv_ts, msgcnt, msgoffset,
        add_order_message{msg.header, msg.new_order_reference_number,
                          copy.buy_sell_indicator, msg.shares, copy.stock,
                          msg.price});

    // ... there is an event, send that to the callback
    auto book_it = std::get<2>(result_add);
    callback_(recv_ts, msg.header, copy.stock,
              book_it->second.get_book_depth());
  } catch (const jb::feed_error& fe) {
    JB_LOG(warning) << "order_replace_message skipping the message: "
                    << fe.what();
  }
}

void jb::itch5::compute_book_depth::handle_unknown(time_point recv_ts,
                                                   unknown_message const& msg) {
  char msgtype = *static_cast<char const*>(msg.buf());
  JB_LOG(error) << "Unknown message type '" << msgtype << "'(" << int(msgtype)
                << ") in msgcnt=" << msg.count()
                << ", msgoffset=" << msg.offset();
}

jb::itch5::compute_book_depth::update_result
jb::itch5::compute_book_depth::handle_add_no_update(
    time_point recv_ts, long msgcnt, std::size_t msgoffset,
    add_order_message const& msg) {
  // First we need to insert the order into the list of active orders ...
  auto position = orders_.emplace(
      msg.order_reference_number,
      order_data{msg.stock, msg.buy_sell_indicator, msg.price, msg.shares});
  if (position.second == false) {
    // ... ooops, this should not happen, there is a problem with the
    // feed, throw an exception to skip the message ...
    // ... strictly speaking the new duplicate message data
    // is in the orders_ now (so no really skip...)
    auto const& data = position.first->second;
    std::ostringstream os;
    os << "duplicate order id=" << msg.order_reference_number
       << " existing.symbol=" << data.stock
       << ", existing.buy_sell_indicator=" << data.buy_sell_indicator
       << ", existing.px=" << data.px << ", existing.qty=" << data.qty
       << ", msg=" << msg;
    throw jb::feed_error(os.str());
  }
  // ... okay, now that the order is inserted, let's make sure there
  // is a book for the symbol, we avoid creating a full order book in
  // the normal case ...
  auto stock_book_it = books_.find(msg.stock);
  if (stock_book_it == books_.end()) {
    auto book_pair = books_.emplace(msg.stock, order_book());
    stock_book_it = book_pair.first; // iterator to the new order book
  }
  // ... add the order to the book, ignore the bool return
  stock_book_it->second.handle_add_order(msg.buy_sell_indicator, msg.price,
                                         msg.shares);
  return std::make_tuple(true, position.first->second, stock_book_it);
}

void jb::itch5::compute_book_depth::handle_reduce(
    time_point recv_ts, long msgcnt, std::size_t msgoffset,
    message_header const& header, std::uint64_t order_reference_number,
    std::uint32_t shares, bool all_shares) {
  try {
    auto result =
        handle_reduce_no_update(recv_ts, msgcnt, msgoffset, header,
                                order_reference_number, shares, all_shares);
    auto const& copy = std::get<1>(result);
    auto stock_book_it = std::get<2>(result);
    callback_(recv_ts, header, copy.stock,
              stock_book_it->second.get_book_depth());
  } catch (const jb::feed_error& fe) {
    JB_LOG(warning) << "handle_reduce skipping the message: " << fe.what();
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
    // feed, throw an exception to skip the message ...
    std::ostringstream os;
    os << "missing order id=" << order_reference_number
       << ", location=" << msgcnt << ":" << msgoffset << ", header=" << header;
    throw jb::feed_error(os.str());
  }
  // ..okay, now that the order is located, find the book for that symbol
  auto& data = position->second;
  auto stock_book_it = books_.find(data.stock);
  JB_ASSERT_THROW(stock_book_it != books_.end()); // runtime error

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
  // ... reduce the quantity at the price...
  stock_book_it->second.handle_order_reduced(copy.buy_sell_indicator, copy.px,
                                             shares);
  return std::make_tuple(true, copy, stock_book_it);
}
