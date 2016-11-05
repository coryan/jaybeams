#include "jb/itch5/compute_book.hpp"

#include <algorithm>

namespace jb {
namespace itch5 {

compute_book::compute_book(callback_type&& cb)
    : callback_(std::forward<callback_type>(cb))
    , books_() {
}

compute_book::compute_book(callback_type const& cb)
    : compute_book(callback_type(cb)) {
}

void compute_book::handle_message(
      time_point recvts, long msgcnt, std::size_t msgoffset,
      stock_directory_message const& msg) {
  // ... create the book and update the map ...
  books_.emplace(msg.stock, order_book());
  // ... only log these messages if we want super-verbose output, there
  // are nearly 8,200 securities in the Nasdaq exchange, we do not
  // want that many log lines on a typical run ...
  JB_LOG(trace) << " " << msgcnt << ":" << msgoffset << " " << msg;
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

} // namespace itch5
} // namespace jb
