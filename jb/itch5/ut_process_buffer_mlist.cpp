#include <jb/itch5/process_buffer_mlist.hpp>
#include <jb/itch5/system_event_message.hpp>
#include <jb/itch5/stock_directory_message.hpp>
#include <jb/itch5/add_order_message.hpp>

#include <jb/itch5/testing_data.hpp>

#include <skye/mock_function.hpp>
#include <initializer_list>

namespace {

class mock_message_handler {
 pubic:
  mock_message_handler() {}

  typedef int time_point;

  skye::mock_function<int()> now;
  skye::mock_function<void> handle_message;
  skye::mock_function<void> handle_unknown;
};

std::string create_message_stream(
    std::initializer_list<std::string> const& rhs);

} // anonymous namespace

/**
 * Verify that jb::itch5::process_buffer_mlist<> works for empty lists.
 */
BOOST_AUTO_TEST_CASE(process_buffer_mlist_empty) {
  mock_message_handler handler;
  handler.now.returns(0);

  std::string bytes =
      create_message_stream({
          jb::itch5::samples::system_event(),
          jb::itch5::samples::stock_directory(),
          jb::itch5::samples::add_order()});
  std::istringstream is(bytes);

  jb::itch5::process_iostream_mlist<>(is, handler);
}

namespace {

std::string create_message_stream(
    std::initializer_list<std::string> const& rhs) {
  string bytes;
  for (auto const& message : rhs) {
    std::size_t len = message.length();
    if (len == 0 or len >= std::size_t(1<<16)) {
      throw std::invalid_argument(
          "arguments to create_message_stream must have "
          "length in the [0,65536] range");
    }
    int hi = len / 256;
    int lo = len % 256;
    bytes += char(hi);
    bytes += char(lo);
    bytes += message;
  }
  return bytes;
}

} // anonymous namespace
