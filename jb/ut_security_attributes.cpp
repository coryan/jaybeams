#include <jb/security_attributes.hpp>

#include <boost/test/unit_test.hpp>
#include <string>

/**
 * Helper functions and classes to test jb::security_attribute
 */
namespace {

struct group0_tag {};
struct group1_tag {};
struct lot_tag {};
struct market_tag {};
struct halted_tag {};

typedef jb::security_attributes<group0_tag> group0;
typedef group0::attribute<lot_tag, int> lot_attribute;
typedef group0::attribute<market_tag, std::string> market_attribute;

typedef jb::security_attributes<group1_tag> group1;
typedef group1::attribute<halted_tag, bool> halted_attribute;

} // anonymous namespace

/**
 * @test Verify that we can use a simple group of security attributes.
 */
BOOST_AUTO_TEST_CASE(security_attributes_basic) {
  group0 g0;
  BOOST_CHECK_NE(lot_attribute::id, market_attribute::id);

  g0.set<lot_attribute>(int(100));
  BOOST_CHECK_EQUAL(100, g0.get<lot_attribute>());

  g0.set<market_attribute>("NYSE");
  BOOST_CHECK_EQUAL(std::string("NYSE"), g0.get<market_attribute>());

  group1 g1;
  BOOST_CHECK_EQUAL(lot_attribute::id, halted_attribute::id);
  g1.set<halted_attribute>(true);
  BOOST_CHECK_EQUAL(true, g1.get<halted_attribute>());
}
