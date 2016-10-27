#include <jb/itch5/compute_inside.hpp>

inline void jb::itch5::compute_inside::call_callback_condition(time_point _ts,
						 message_header const& _msg_header,
						 stock_t const& _stock,
						 half_quote const& _best_bid,
						 half_quote const& _best_offer,
						 book_depth_t const _book_depth,
						 bool const _is_inside) {
  // reports only when _is_inside
  if (_is_inside)
    callback_(_ts, _msg_header, _stock, _best_bid, _best_offer);
}
