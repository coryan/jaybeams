#include <jb/itch5/add_order_message.hpp>
#include <jb/itch5/process_buffer_mlist.hpp>
#include <jb/itch5/stock_directory_message.hpp>
#include <jb/itch5/system_event_message.hpp>

#include <jb/itch5/testing/data.hpp>

#include <jb/gmock/init.hpp>
#include <boost/test/unit_test.hpp>

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

} // anonymous namespace

/**
 * Verify that jb::itch5::process_buffer_mlist<> works for empty lists.
 */
BOOST_AUTO_TEST_CASE(process_buffer_mlist_empty) {
  mock_message_handler handler;
  using namespace ::testing;
  EXPECT_CALL(handler, handle_unknown(Eq(42), _)).Times(1);

  auto p = jb::itch5::testing::system_event();
  jb::itch5::process_buffer_mlist<mock_message_handler>::process(
      handler, 42, 2, 100, p.first, p.second);
}

/**
 * Verify that jb::itch5::process_buffer_mlist<> works for a list with
 * a single element.
 */
BOOST_AUTO_TEST_CASE(process_buffer_mlist_single) {
  mock_message_handler handler;
  using namespace ::testing;

  {
    auto p = jb::itch5::testing::system_event();
    EXPECT_CALL(
        handler,
        handle_message(42, _, _, An<jb::itch5::system_event_message const&>()))
        .Times(1);
    jb::itch5::process_buffer_mlist<
        mock_message_handler, jb::itch5::system_event_message>::
        process(handler, 42, 2, 100, p.first, p.second);
  }

  {
    auto p = jb::itch5::testing::stock_directory();
    EXPECT_CALL(handler, handle_unknown(4242, _)).Times(1);
    jb::itch5::process_buffer_mlist<
        mock_message_handler, jb::itch5::system_event_message>::
        process(handler, 4242, 3, 200, p.first, p.second);
  }
}

/**
 * Verify that jb::itch5::process_buffer_mlist<> works for a list with
 * 3 elements.
 */
BOOST_AUTO_TEST_CASE(process_buffer_mlist_3) {
  mock_message_handler handler;
  using namespace ::testing;

  typedef jb::itch5::process_buffer_mlist<
      mock_message_handler, jb::itch5::system_event_message,
      jb::itch5::stock_directory_message, jb::itch5::add_order_message>
      tested;

  {
    EXPECT_CALL(
        handler,
        handle_message(42, _, _, An<jb::itch5::system_event_message const&>()))
        .Times(1);
    auto p = jb::itch5::testing::system_event();
    tested::process(handler, 42, 2, 100, p.first, p.second);
  }
  {
    EXPECT_CALL(
        handler, handle_message(
                     43, _, _, An<jb::itch5::stock_directory_message const&>()))
        .Times(1);
    auto p = jb::itch5::testing::stock_directory();
    tested::process(handler, 43, 3, 120, p.first, p.second);
  }
  {
    EXPECT_CALL(
        handler,
        handle_message(44, _, _, An<jb::itch5::add_order_message const&>()))
        .Times(1);
    auto p = jb::itch5::testing::add_order();
    tested::process(handler, 44, 4, 140, p.first, p.second);
  }
}
