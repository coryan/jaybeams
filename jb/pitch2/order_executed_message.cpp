#include <jb/pitch2/order_executed_message.hpp>

#include <boost/io/ios_state.hpp>
#include <ostream>

namespace jb {
namespace pitch2 {

std::ostream& operator<<(std::ostream& os, order_executed_message const& x) {
  return os << "length=" << static_cast<int>(x.length.value())
            << ",message_type=" << static_cast<int>(x.message_type.value())
            << ",time_offset=" << x.time_offset.value()
            << ",order_id=" << x.order_id.value()
            << ",executed_quantity=" << x.executed_quantity.value()
            << ",execution_id=" << x.execution_id.value();
}

} // namespace pitch2
} // namespace jb
