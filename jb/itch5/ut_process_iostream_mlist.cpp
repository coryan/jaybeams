#include <jb/itch5/add_order_message.hpp>
#include <jb/itch5/process_iostream_mlist.hpp>
#include <jb/itch5/stock_directory_message.hpp>
#include <jb/itch5/system_event_message.hpp>

#include <jb/itch5/testing/data.hpp>

#include <jb/gmock/init.hpp>
#include <boost/test/unit_test.hpp>

#include <initializer_list>

namespace {

class mock_message_handler {
public:
  mock_message_handler() {
  }

  typedef int time_point;

  MOCK_METHOD0(now, int());
  MOCK_METHOD2(
      handle_unknown, void(int const&, jb::itch5::unknown_message const&));
  MOCK_METHOD4(
      handle_message,
      void(
          int const&, std::uint64_t msgcnt, std::size_t msgoffset,
          jb::itch5::system_event_message const&));
  MOCK_METHOD4(
      handle_message,
      void(
          int const&, std::uint64_t msgcnt, std::size_t msgoffset,
          jb::itch5::stock_directory_message const&));
  MOCK_METHOD4(
      handle_message,
      void(
          int const&, std::uint64_t msgcnt, std::size_t msgoffset,
          jb::itch5::add_order_message const&));
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
  using namespace ::testing;
  EXPECT_CALL(handler, now()).WillRepeatedly(Return(0));

  std::string bytes = create_message_stream(
      {jb::itch5::testing::system_event(),
       jb::itch5::testing::stock_directory(),
       jb::itch5::testing::stock_directory(),
       jb::itch5::testing::stock_directory(), jb::itch5::testing::add_order(),
       jb::itch5::testing::add_order(), jb::itch5::testing::add_order(),
       jb::itch5::testing::add_order(), jb::itch5::testing::trade(),
       jb::itch5::testing::system_event()});
  std::istringstream is(bytes);

  EXPECT_CALL(handler, now()).Times(21);
  EXPECT_CALL(
      handler,
      handle_message(_, _, _, An<jb::itch5::add_order_message const&>()))
      .Times(4);
  EXPECT_CALL(
      handler,
      handle_message(_, _, _, An<jb::itch5::stock_directory_message const&>()))
      .Times(3);
  EXPECT_CALL(
      handler,
      handle_message(_, _, _, An<jb::itch5::system_event_message const&>()))
      .Times(2);
  EXPECT_CALL(handler, handle_unknown(_, _)).Times(1);
  jb::itch5::process_iostream_mlist<
      mock_message_handler, jb::itch5::system_event_message,
      jb::itch5::stock_directory_message, jb::itch5::add_order_message>(
      is, handler);
}

/**
 * @test Verify that jb::itch5::process_iostream_mlist<> exits
 * gracefully on I/O errors.
 */
BOOST_AUTO_TEST_CASE(process_iostream_mlist_errors) {
  mock_message_handler handler;

  std::string bytes =
      create_message_stream({jb::itch5::testing::system_event(),
                             jb::itch5::testing::stock_directory(),
                             jb::itch5::testing::stock_directory(),
                             jb::itch5::testing::stock_directory(),
                             jb::itch5::testing::stock_directory()});
  std::istringstream is(bytes);

  int count = 0;
  // Simulate a iostream failure on the 5 read() ...
  using namespace ::testing;
  EXPECT_CALL(handler, now()).WillRepeatedly(Invoke([&is, &count]() {
    if (++count == 5) {
      is.setstate(std::ios::failbit);
    }
    return 0;
  }));

  EXPECT_CALL(handler, now()).Times(5);
  EXPECT_CALL(
      handler,
      handle_message(_, _, _, An<jb::itch5::system_event_message const&>()))
      .Times(1);
  EXPECT_CALL(
      handler,
      handle_message(_, _, _, An<jb::itch5::stock_directory_message const&>()))
      .Times(1);
  EXPECT_CALL(handler, handle_unknown(_, _)).Times(0);
  jb::itch5::process_iostream_mlist<
      mock_message_handler, jb::itch5::system_event_message,
      jb::itch5::stock_directory_message, jb::itch5::add_order_message>(
      is, handler);
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
