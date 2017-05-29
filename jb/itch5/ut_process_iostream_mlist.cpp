#include <jb/itch5/add_order_message.hpp>
#include <jb/itch5/process_iostream_mlist.hpp>
#include <jb/itch5/stock_directory_message.hpp>
#include <jb/itch5/system_event_message.hpp>

#include <jb/itch5/testing/data.hpp>

#include <skye/mock_function.hpp>
#include <skye/mock_template_function.hpp>
#include <initializer_list>

namespace {

class mock_message_handler {
public:
  mock_message_handler() {
  }

  typedef int time_point;

  skye::mock_function<int()> now;
  skye::mock_function<void(int const&, jb::itch5::unknown_message const&)>
      handle_unknown;

  skye::mock_template_function<void> handle_message;
};

std::string create_message_stream(
    std::initializer_list<std::pair<char const*, std::size_t>> const& rhs);

} // anonymous namespace

/**
 * @test Verify that jb::itch5::process_iostream_mlist<> works as expected.
 */
BOOST_AUTO_TEST_CASE(process_iostream_mlist_simple) {
  // TODO(#5) this is a really trivial test, its main purpose is to
  // get the code to compile.  The functions are tested elsewhere, but
  // this is a big shameful.
  mock_message_handler handler;
  handler.now.returns(0);

  std::string bytes = create_message_stream(
      {jb::itch5::testing::system_event(),
       jb::itch5::testing::stock_directory(),
       jb::itch5::testing::stock_directory(),
       jb::itch5::testing::stock_directory(), jb::itch5::testing::add_order(),
       jb::itch5::testing::add_order(), jb::itch5::testing::add_order(),
       jb::itch5::testing::add_order(), jb::itch5::testing::trade(),
       jb::itch5::testing::system_event()});
  std::istringstream is(bytes);

  jb::itch5::process_iostream_mlist<
      mock_message_handler, jb::itch5::system_event_message,
      jb::itch5::stock_directory_message, jb::itch5::add_order_message>(
      is, handler);

  handler.now.check_called().exactly(21);
  handler.handle_message.check_called().exactly(9);
  handler.handle_unknown.check_called().exactly(1);
}

/**
 * @test Verify that jb::itch5::process_iostream_mlist<> exits
 * gracefully on I/O errors.
 */
BOOST_AUTO_TEST_CASE(process_iostream_mlist_errors) {
  mock_message_handler handler;

  std::string bytes = create_message_stream(
      {jb::itch5::testing::system_event(),
       jb::itch5::testing::stock_directory(),
       jb::itch5::testing::stock_directory(),
       jb::itch5::testing::stock_directory(),
       jb::itch5::testing::stock_directory()});
  std::istringstream is(bytes);

  int count = 0;
  handler.now.action([&is, &count]() {
    if (++count == 5) {
      is.setstate(std::ios::failbit);
    }
    return 0;
  });

  jb::itch5::process_iostream_mlist<
      mock_message_handler, jb::itch5::system_event_message,
      jb::itch5::stock_directory_message, jb::itch5::add_order_message>(
      is, handler);

  // We expect invocations for count == 0 and count == 1
  handler.now.check_called().exactly(5);
  handler.handle_message.check_called().exactly(2);
  handler.handle_unknown.check_called().exactly(0);
}

namespace {

std::string create_message_stream(
    std::initializer_list<std::pair<char const*, std::size_t>> const& rhs) {
  std::string bytes;
  for (auto const& p : rhs) {
    std::size_t len = p.second;
    if (len == 0 or len >= std::size_t(1 << 16)) {
      throw std::invalid_argument(
          "arguments to create_message_stream must have "
          "length in the [0,65536] range");
    }
    int hi = len / 256;
    int lo = len % 256;
    bytes.push_back(char(hi));
    bytes.push_back(char(lo));
    bytes.append(p.first, p.second);
  }
  return bytes;
}

} // anonymous namespace
