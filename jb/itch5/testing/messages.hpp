#ifndef jb_itch5_testing_messages_hpp
#define jb_itch5_testing_messages_hpp

#include <jb/itch5/stock_directory_message.hpp>

namespace jb {
namespace itch5 {
namespace testing {

/// Create a dummy jb::itch5::stock_directory_message for testing.
stock_directory_message create_stock_directory(char const* symbol);

} // namespace testing
} // namespace itch5
} // namespace jb

#endif // jb_itch5_testing_messages_hpp
