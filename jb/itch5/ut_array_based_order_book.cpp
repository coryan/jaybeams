#include <jb/itch5/array_based_order_book.hpp>
#include <jb/itch5/ut_type_based_order_book.hpp>

#include <boost/test/unit_test.hpp>

/**
 * @test Trivial verification that array_based_order_book works as expected.
 */
BOOST_AUTO_TEST_CASE(array_based_order_book_trivial) {
  jb::itch5::array_based_order_book tested;
  jb::itch5::testing::test_order_book_type_trivial(tested);
}

/**
 * @test Verify that array_based_order_book handles add and reduce as expected.
 */
BOOST_AUTO_TEST_CASE(array_based_order_book_test) {
  jb::itch5::array_based_order_book tested;
  jb::itch5::testing::test_order_book_type_add_reduce(tested);
}

/**
 * @test Verify that array_based_order_book handles errors as expected.
 */
BOOST_AUTO_TEST_CASE(array_based_order_book_errors) {
  jb::itch5::array_based_order_book tested;
  jb::itch5::testing::test_order_book_type_errors(tested);
}
