#include <jb/itch5/cross_type.hpp>
#include <boost/test/unit_test.hpp>

/**
 * @test Verify that cross_type_t works as expected.
 */
BOOST_AUTO_TEST_CASE(simple_cross_type) {
  using namespace jb::itch5;
  BOOST_CHECK_NO_THROW(cross_type_t(u'O'));
  BOOST_CHECK_NO_THROW(cross_type_t(u'C'));
  BOOST_CHECK_NO_THROW(cross_type_t(u'H'));
  BOOST_CHECK_NO_THROW(cross_type_t(u'I'));
  BOOST_CHECK_THROW(cross_type_t(u'*'), std::runtime_error);
}
