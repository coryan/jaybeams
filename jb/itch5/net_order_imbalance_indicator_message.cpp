#include <jb/itch5/net_order_imbalance_indicator_message.hpp>

#include <iostream>

constexpr int jb::itch5::net_order_imbalance_indicator_message::message_type;

std::ostream& jb::itch5::operator<<(
    std::ostream& os, net_order_imbalance_indicator_message const& x) {
  return os << x.header
            << ",paired_shares=" << x.paired_shares
            << ",imbalance_shares=" << x.imbalance_shares
            << ",imbalance_direction=" << x.imbalance_direction
            << ",stock=" << x.stock
            << ",far_price=" << x.far_price
            << ",near_price=" << x.near_price
            << ",current_reference_price=" << x.current_reference_price
            << ",cross_type=" << x.cross_type
            << ",price_variation_indicator=" << x.price_variation_indicator
      ;
}
