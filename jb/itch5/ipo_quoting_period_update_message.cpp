#include "jb/itch5/ipo_quoting_period_update_message.hpp"

#include <iostream>

constexpr int jb::itch5::ipo_quoting_period_update_message::message_type;

std::ostream& jb::itch5::
operator<<(std::ostream& os, ipo_quoting_period_update_message const& x) {
  return os << x.header << ",stock=" << x.stock
            << ",ipo_quotation_release_time=" << x.ipo_quotation_release_time
            << ",ipo_quotation_release_qualifier="
            << x.ipo_quotation_release_qualifier
            << ",ipo_price=" << x.ipo_price;
}
