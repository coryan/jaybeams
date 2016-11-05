#include "jb/itch5/compute_book.hpp"

namespace jb {
namespace itch5 {

compute_book::compute_book(callback_type&& cb)
    : callback_(std::forward<callback_type>(cb)) {
}

compute_book::compute_book(callback_type const& cb)
    : compute_book(callback_type(cb)) {
}

std::ostream& operator<<(std::ostream& os, compute_book::book_update const& x) {
  return os << "{" << x.stock << "," << x.buy_sell_indicator << "," << x.px
            << "," << x.qty << "}";
}

} // namespace itch5
} // namespace jb
