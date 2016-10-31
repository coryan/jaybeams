#include <jb/itch5/add_order_message.hpp>
#include <jb/itch5/process_buffer_mlist.hpp>
#include <jb/itch5/stock_directory_message.hpp>
#include <jb/itch5/system_event_message.hpp>

#include <jb/itch5/testing_data.hpp>

#include <skye/mock_function.hpp>
#include <skye/mock_template_function.hpp>

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

} // anonymous namespace

/**
 * Verify that jb::itch5::process_buffer_mlist<> works for empty lists.
 */
BOOST_AUTO_TEST_CASE(process_buffer_mlist_empty) {
  mock_message_handler handler;

  auto p = jb::itch5::testing::system_event();
  jb::itch5::process_buffer_mlist<mock_message_handler>::process(
      handler, 42, 2, 100, p.first, p.second);
  handler.handle_unknown.require_called().once();
  BOOST_CHECK_EQUAL(std::get<0>(handler.handle_unknown.at(0)), 42);
}

/**
 * Verify that jb::itch5::process_buffer_mlist<> works for a list with
 * a single element.
 */
BOOST_AUTO_TEST_CASE(process_buffer_mlist_single) {
  mock_message_handler handler;

  {
    auto p = jb::itch5::testing::system_event();
    jb::itch5::process_buffer_mlist<mock_message_handler,
                                    jb::itch5::system_event_message>::
        process(handler, 42, 2, 100, p.first, p.second);
  }
  handler.handle_message.require_called().once();

  {
    auto p = jb::itch5::testing::stock_directory();
    jb::itch5::process_buffer_mlist<mock_message_handler,
                                    jb::itch5::system_event_message>::
        process(handler, 4242, 3, 200, p.first, p.second);
  }
  handler.handle_message.check_called().once();
  handler.handle_unknown.require_called().once();
  BOOST_CHECK_EQUAL(std::get<0>(handler.handle_unknown.at(0)), 4242);
}

/**
 * Verify that jb::itch5::process_buffer_mlist<> works for a list with
 * 3 elements.
 */
BOOST_AUTO_TEST_CASE(process_buffer_mlist_3) {
  mock_message_handler handler;

  typedef jb::itch5::process_buffer_mlist<
      mock_message_handler, jb::itch5::system_event_message,
      jb::itch5::stock_directory_message, jb::itch5::add_order_message>
      tested;

  {
    auto p = jb::itch5::testing::system_event();
    tested::process(handler, 42, 2, 100, p.first, p.second);
  }
  {
    auto p = jb::itch5::testing::stock_directory();
    tested::process(handler, 43, 3, 120, p.first, p.second);
  }
  {
    auto p = jb::itch5::testing::add_order();
    tested::process(handler, 44, 4, 140, p.first, p.second);
  }
  handler.handle_message.require_called().exactly(3);
}
