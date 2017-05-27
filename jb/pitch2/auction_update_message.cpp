#include <jb/pitch2/auction_update_message.hpp>

#include <ostream>

namespace jb {
namespace pitch2 {

std::ostream& operator<<(std::ostream& os, auction_update_message const& x) {
  return os << "length=" << static_cast<int>(x.length.value())
            << ",message_type=" << static_cast<int>(x.message_type.value())
            << ",time_offset=" << x.time_offset.value()
            << ",stock_symbol=" << std::string(x.stock_symbol, 8)
            << ",auction_type=" << static_cast<char>(x.auction_type.value())
            << ",reference_price=" << x.reference_price.value()
            << ",buy_shares=" << x.buy_shares.value()
            << ",sell_shares=" << x.sell_shares.value()
            << ",indicative_price=" << x.indicative_price.value()
            << ",auction_only_price=" << x.auction_only_price.value();
}

} // namespace pitch2
} // namespace jb
