#include <jb/config_files_location.hpp>
#include <jb/config_object.hpp>

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
      , bar(desc("bar"), this) {
  }
  config_object_constructors(simple);

  jb::config_attribute<simple, std::string> foo;
  jb::config_attribute<simple, std::string> bar;
};

class config : public jb::config_object {
public:
  config()
      : input(desc("input"), this)
      , outputs(desc("outputs"), this) {
  }
  config_object_constructors(config);

  jb::config_attribute<config, std::string> input;
  jb::config_attribute<config, std::vector<simple>> outputs;
};

class nested : public jb::config_object {
public:
  nested()
      : baz(desc("baz", "config"), this) {
  }
  config_object_constructors(nested);
  jb::config_attribute<nested, config> baz;
};

class verynested : public jb::config_object {
public:
  verynested()
      : qux(desc("qux", "nested"), this)
      , quz(desc("quz", "nested"), this) {
  }
  config_object_constructors(verynested);
  jb::config_attribute<verynested, nested> qux;
  jb::config_attribute<verynested, nested> quz;
};

// Convenience functions for the unit tests.
std::ostream& operator<<(std::ostream& os, simple const& x) {
  return os << "{foo=" << x.foo() << ", bar=" << x.bar() << "}";
}

bool operator==(simple const& lhs, simple const& rhs) {
  return lhs.foo() == rhs.foo() and lhs.bar() == rhs.bar();
}

bool operator!=(simple const& lhs, simple const& rhs) {
  return not(lhs == rhs);
}
} // anonymous namespace

/**
 * @test Verify we can merge configurations with vectors.
 */
BOOST_AUTO_TEST_CASE(config_object_vector) {
  char const contents[] = R"""(
# YAML overrides
input: bar
outputs: [ {bar: 6} ]
)""";

  BOOST_TEST_MESSAGE("Applying overrides from\n" << contents << "\n");
  config tested;
  tested.input("foo").outputs(std::vector<simple>(
      {simple().foo("1").bar("2"), simple().foo("3").bar("4")}));
  BOOST_TEST_MESSAGE("Initial Configuration\n" << tested << "\n");
  std::istringstream is(contents);
  int argc = 0;
  tested.load_overrides(argc, nullptr, is);

  BOOST_TEST_MESSAGE("Post-Override Configuration\n" << tested << "\n");

  BOOST_CHECK_EQUAL(tested.input(), "bar");
  std::vector<simple> expected(
      {simple().foo("1").bar("6"), simple().foo("3").bar("4")});
  BOOST_CHECK_EQUAL_COLLECTIONS(
      tested.outputs().begin(), tested.outputs().end(), expected.begin(),
      expected.end());
}

/**
 * @test Verify we can merge configurations with larger sequences.
 */
BOOST_AUTO_TEST_CASE(config_object_vector_longer) {
  char const contents[] = R"""(
# YAML overrides
input: bar
outputs: [ {bar: 6}, {}, {foo: 7, bar: 8} ]
)""";

  BOOST_TEST_MESSAGE("Applying overrides from\n" << contents << "\n");
  config tested;
  tested.input("foo").outputs(std::vector<simple>(
      {simple().foo("1").bar("2"), simple().foo("3").bar("4")}));
  BOOST_TEST_MESSAGE("Initial Configuration\n" << tested << "\n");
  std::istringstream is(contents);
  int argc = 0;
  tested.load_overrides(argc, nullptr, is);

  BOOST_TEST_MESSAGE("Post-Override Configuration\n" << tested << "\n");

  BOOST_CHECK_EQUAL(tested.input(), "bar");
  std::vector<simple> expected({simple().foo("1").bar("6"),
                                simple().foo("3").bar("4"),
                                simple().foo("7").bar("8")});
  BOOST_CHECK_EQUAL_COLLECTIONS(
      tested.outputs().begin(), tested.outputs().end(), expected.begin(),
      expected.end());
}

/**
 * @test Verify we can merge configurations with vectors by class.
 */
BOOST_AUTO_TEST_CASE(config_object_vector_by_class) {
  char const contents[] = R"""(
# YAML overrides
# This applies to all objects of type 'config'
:config:
  input: bar
  outputs: [ {bar: 22} ]
qux:
  baz:
    input: qux.baz
    outputs: [ {foo: 1, bar: 2}, {foo: 3} ]
quz:
  :config:
    input: quz.baz
    outputs: [ {foo: 11}, {foo: 33, bar:44} ]
  baz:
    outputs: [ {}, {bar: 4}, {foo: 5, bar: 6} ]
)""";

  verynested tested;
  std::istringstream is(contents);
  int argc = 0;
  tested.load_overrides(argc, nullptr, is);

  BOOST_CHECK_EQUAL(tested.qux().baz().input(), "qux.baz");
  std::vector<simple> expected(
      {simple().foo("1").bar("2"), simple().foo("3").bar("")});
  BOOST_CHECK_EQUAL_COLLECTIONS(
      tested.qux().baz().outputs().begin(), tested.qux().baz().outputs().end(),
      expected.begin(), expected.end());

  BOOST_CHECK_EQUAL(tested.quz().baz().input(), "quz.baz");
  expected.assign({simple().foo("11").bar("22"), simple().foo("33").bar("4"),
                   simple().foo("5").bar("6")});
  BOOST_CHECK_EQUAL_COLLECTIONS(
      tested.quz().baz().outputs().begin(), tested.quz().baz().outputs().end(),
      expected.begin(), expected.end());
}
