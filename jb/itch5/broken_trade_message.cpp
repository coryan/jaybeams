#include <jb/itch5/broken_trade_message.hpp>

#include <iostream>

constexpr int jb::itch5::broken_trade_message::message_type;

std::ostream& jb::itch5::operator<<(std::ostream& os,
                                    broken_trade_message const& x) {
  return os << x.header << ",match_number=" << x.match_number;
}
