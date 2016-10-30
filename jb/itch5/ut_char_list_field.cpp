#include <jb/itch5/char_list_field.hpp>

#include <boost/test/unit_test.hpp>

/**
 * Define types used in the tests.
 */
namespace {
typedef jb::itch5::char_list_field<u'Y', u'N', u' '> tested;
} // anonymous namespace

/**
 * @test Verify that jb::itch5::decoder works for jb::itch5::char_list_field.
 */
BOOST_AUTO_TEST_CASE(decode_char_list_field) {
  using jb::itch5::decoder;

  char buffer[32] = {u'Y', u'N', u' ', u'A', u'B'};

  auto actual = decoder<true, tested>::r(16, buffer, 0);
  BOOST_CHECK_EQUAL(actual, u'Y');

  actual = decoder<false, tested>::r(16, buffer, 0);
  BOOST_CHECK_EQUAL(actual, u'Y');

  actual = decoder<true, tested>::r(16, buffer, 1);
  BOOST_CHECK_EQUAL(actual, u'N');
  actual = decoder<false, tested>::r(16, buffer, 1);
  BOOST_CHECK_EQUAL(actual, u'N');

  BOOST_CHECK_THROW((decoder<true, tested>::r(16, buffer, 3)),
                    std::runtime_error);
  BOOST_CHECK_NO_THROW((decoder<false, tested>::r(16, buffer, 3)));

  BOOST_CHECK_THROW((decoder<true, tested>::r(16, buffer, 16)),
                    std::runtime_error);
  BOOST_CHECK_NO_THROW((decoder<false, tested>::r(16, buffer, 16)));
}

/**
 * @test Verify that jb::itch5::char_list_field iostream operator
 * works as expected.
 */
BOOST_AUTO_TEST_CASE(stream_char_list_field) {
  using jb::itch5::decoder;

  {
    std::ostringstream os;
    os << tested(u'Y');
    BOOST_CHECK_EQUAL(os.str(), "Y");
  }

  {
    char buffer[32] = {u'\0'};
    auto actual = decoder<false, tested>::r(16, buffer, 0);

    std::ostringstream os;
    os << actual;
    BOOST_CHECK_EQUAL(os.str(), ".(0)");
  }

  {
    char buffer[32] = {u'\n'};
    auto actual = decoder<false, tested>::r(16, buffer, 0);

    std::ostringstream os;
    os << actual;
    BOOST_CHECK_EQUAL(os.str(), ".(10)");
  }
}
