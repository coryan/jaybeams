#include <jb/itch5/map_based_order_book.hpp>
#include <jb/itch5/testing/ut_type_based_order_book.hpp>

#include <boost/test/unit_test.hpp>

/**
 * @test Trivial verification that map_based_order_book works as expected.
 */
BOOST_AUTO_TEST_CASE(map_based_order_book_trivial) {
  using namespace jb::itch5;
  testing::test_order_book_type_trivial<map_based_order_book>();
}

/**
 * @test Verify that map_based_order_book handles add and reduce as expected.
 */
BOOST_AUTO_TEST_CASE(map_based_order_book_test) {
  using namespace jb::itch5;
  testing::test_order_book_type_add_reduce<map_based_order_book>();
}

/**
 * @test Verify that map_based_order_book handles errors as expected.
 */
BOOST_AUTO_TEST_CASE(map_based_order_book_errors) {
  using namespace jb::itch5;
  testing::test_order_book_type_errors<map_based_order_book>();
}

/**
 * @test Verify that map_based_order_book::config works as expected.
 */
BOOST_AUTO_TEST_CASE(map_based_order_book_config_simple) {
  using namespace jb::itch5;

  BOOST_CHECK_NO_THROW(map_based_order_book::config().validate());
}
