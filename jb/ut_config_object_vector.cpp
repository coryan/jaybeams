#include <jb/config_object.hpp>
#include <jb/config_files_location.hpp>

#include <boost/test/unit_test.hpp>
#include <cstdlib>
#include <fstream>
#include <string>
#include <vector>

/**
 * Helper classes to test jb::config_object and jb::config_attribute
 */
namespace {
class simple : public jb::config_object {
 public:
  simple()
      : foo(desc("foo"), this)
      , bar(desc("bar"), this)
  {}
  config_object_constructors(simple);

  jb::config_attribute<simple,std::string> foo;
  jb::config_attribute<simple,std::string> bar;
};

class config : public jb::config_object {
 public:
  config()
      : input(desc("input"), this)
      , outputs(desc("outputs"), this)
  {}
  config_object_constructors(config);

  jb::config_attribute<config,std::string> input;
  jb::config_attribute<config,std::vector<simple>> outputs;
};
        
// Convenience functions for the unit tests.
std::ostream& operator<<(std::ostream& os, simple const& x) {
  return os << "{foo=" << x.foo() << ", bar=" << x.bar() << "}";
}

bool operator==(simple const& lhs, simple const& rhs) {
  return lhs.foo() == rhs.foo() and lhs.bar() == rhs.bar();
}

bool operator!=(simple const& lhs, simple const& rhs) {
  return !(lhs == rhs);
}
} // anonymous namespace

/**
 * @test Verify we can merge configurations with vectors.
 */
BOOST_AUTO_TEST_CASE(config_object_vector) {
  char const contents[] = R"""(
# YAML overrides
# This applies to all objects of type 'config'
input: bar
outputs: [ {bar: 6} ]
)""";

  BOOST_MESSAGE("Applying overrides from\n"
                << contents
                << "\n");
  config tested;
  tested
      .input("foo")
      .outputs(std::vector<simple>(
        { simple().foo("1").bar("2"), simple().foo("3").bar("4") }));
  BOOST_MESSAGE("Initial Configuration\n" << tested << "\n");
  std::istringstream is(contents);
  int argc = 0;
  tested.load_overrides(argc, nullptr, is);

  BOOST_MESSAGE("Post-Override Configuration\n" << tested << "\n");

  BOOST_CHECK_EQUAL(tested.input(), "bar");
  std::vector<simple> expected(
      { simple().foo("1").bar("6"), simple().foo("3").bar("4")});
  BOOST_CHECK_EQUAL_COLLECTIONS(
      tested.outputs().begin(), tested.outputs().end(),
      expected.begin(), expected.end());
}

  
