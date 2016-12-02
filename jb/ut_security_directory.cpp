#include <jb/security.hpp>
#include <jb/security_directory.hpp>

#include <boost/test/unit_test.hpp>

/**
 * Helper functions and classes to test jb::security_attribute
 */
namespace {

struct lot_tag {};
struct valid_tag {};

using lot = jb::security_directory_attributes::attribute<lot_tag, int>;
using valid = jb::security_directory_attributes::attribute<valid_tag, bool>;

} // anonymous namespace

/**
 * @test Verify that we can create a security directory and use it.
 */
BOOST_AUTO_TEST_CASE(security_directory_basic) {
  auto directory = jb::security_directory::create_directory();
  auto goog = directory->insert("GOOG");
  auto msft = directory->insert("MSFT");
  auto trash = directory->insert("HSART");

  directory->set_attribute<lot>(goog, 100);
  directory->set_attribute<lot>(msft, 100);
  directory->set_attribute<lot>(trash, 100);
  directory->set_attribute<valid>(goog, true);
  directory->set_attribute<valid>(msft, true);
  directory->set_attribute<valid>(trash, false);

  BOOST_CHECK_EQUAL(goog.str(), "GOOG");
  BOOST_CHECK_EQUAL(msft.str(), "MSFT");
  BOOST_CHECK_EQUAL(trash.str(), "HSART");

  BOOST_CHECK_EQUAL(goog.get<lot>(), 100);
  BOOST_CHECK_EQUAL(msft.get<lot>(), 100);
  BOOST_CHECK_EQUAL(trash.get<lot>(), 100);

  BOOST_CHECK_EQUAL(goog.get<valid>(), true);
  BOOST_CHECK_EQUAL(msft.get<valid>(), true);
  BOOST_CHECK_EQUAL(trash.get<valid>(), false);

  auto tmp = directory->insert("GOOG");
  BOOST_CHECK_EQUAL(tmp.str(), "GOOG");
}
