#include <jb/itch5/char_list_validator.hpp>

#include <boost/test/unit_test.hpp>

/**
 * @test Verify that the trivial validators work as expected.
 */
BOOST_AUTO_TEST_CASE(base_validator) {
  jb::itch5::char_list_validator<false> bfalse;
  BOOST_CHECK_NO_THROW(bfalse(u'Y'));

  jb::itch5::char_list_validator<true> btrue;
  BOOST_CHECK_THROW(btrue(u'Y'), std::runtime_error);
}

/**
 * @test Verify that the disabled validator works as expected.
 */
BOOST_AUTO_TEST_CASE(disabled_validator) {
  jb::itch5::char_list_validator<false, u'A', u'B', u'C'> bfalse;
  BOOST_CHECK_NO_THROW(bfalse(u'A'));
  BOOST_CHECK_NO_THROW(bfalse(u'B'));
  BOOST_CHECK_NO_THROW(bfalse(u'C'));
  BOOST_CHECK_NO_THROW(bfalse(u'Y'));
}

/**
 * @test Verify that the enabled validator works as expected.
 */
BOOST_AUTO_TEST_CASE(enabled_validator) {
  jb::itch5::char_list_validator<true, u'A', u'B', u'C'> btrue;
  BOOST_CHECK_NO_THROW(btrue(u'A'));
  BOOST_CHECK_NO_THROW(btrue(u'B'));
  BOOST_CHECK_NO_THROW(btrue(u'C'));
  BOOST_CHECK_THROW(btrue(u'Y'), std::runtime_error);
}
