#include <jb/config_files_location.hpp>
#include <jb/config_object.hpp>

#include <boost/filesystem.hpp>
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
      : foo(this)
      , bar(this, "default value")
      , baz(this, 123) {
  }
  config_object_constructors(simple);

  jb::config_attribute<simple, std::string> foo;
  jb::config_attribute<simple, std::string> bar;
  jb::config_attribute<simple, int> baz;
};

class complex : public jb::config_object {
public:
  complex()
      : base(this)
      , names(this, {"one", "two", "3"}) {
  }
  config_object_constructors(complex);

  jb::config_attribute<complex, simple> base;
  jb::config_attribute<complex, std::vector<std::string>> names;
};

class test_variadic : public jb::config_object {
public:
  test_variadic()
      : foo(this, 1, 2) {
  }
  config_object_constructors(test_variadic);

  jb::config_attribute<test_variadic, std::pair<int, int>> foo;
};
}

/**
 * @test Verify we can create simple jb::config_attribute objects
 */
BOOST_AUTO_TEST_CASE(config_attribute_simple) {
  simple tested;

  BOOST_CHECK_EQUAL(tested.foo(), std::string(""));
  BOOST_CHECK_EQUAL(tested.bar(), std::string("default value"));
  BOOST_CHECK_EQUAL(tested.baz(), 123);

  tested.foo("new value").baz(456);
  BOOST_CHECK_EQUAL(tested.foo(), std::string("new value"));
  BOOST_CHECK_EQUAL(tested.baz(), 456);
}

/**
 * @test Verify we can create more complex jb::config_attribute objects
 */
BOOST_AUTO_TEST_CASE(config_attribute_complex) {
  complex tested;

  BOOST_CHECK_EQUAL(tested.base().foo(), std::string(""));
  BOOST_CHECK_EQUAL(tested.base().bar(), std::string("default value"));
  BOOST_CHECK_EQUAL(tested.base().baz(), 123);
  BOOST_REQUIRE_EQUAL(tested.names().size(), 3);
  BOOST_CHECK_EQUAL(tested.names()[0], "one");
  BOOST_CHECK_EQUAL(tested.names()[1], "two");
  BOOST_CHECK_EQUAL(tested.names()[2], "3");

  auto tmp = tested.base();
  tmp.foo("new value").baz(456);
  tested.base(std::move(tmp));

  complex other(tested);
  BOOST_CHECK_EQUAL(other.base().foo(), std::string("new value"));
  BOOST_CHECK_EQUAL(other.base().baz(), 456);
}

/**
 * @test Verify we can copy and assign complex config objects.
 */
BOOST_AUTO_TEST_CASE(config_attribute_complex_copy) {
  complex src;

  auto tmp = src.base();
  tmp.foo("new value").baz(456);
  src.base(std::move(tmp));
  src.names(std::vector<std::string>({"1", "2", "three"}));

  complex tested;
  tested = std::move(src);
  BOOST_REQUIRE_EQUAL(tested.names().size(), 3);
  BOOST_CHECK_EQUAL(tested.names()[0], "1");
  BOOST_CHECK_EQUAL(tested.names()[1], "2");
  BOOST_CHECK_EQUAL(tested.names()[2], "three");
  BOOST_CHECK_EQUAL(src.names().size(), 0);

  complex other(std::move(tested));
  BOOST_REQUIRE_EQUAL(other.names().size(), 3);
  BOOST_CHECK_EQUAL(other.names()[0], "1");
  BOOST_CHECK_EQUAL(other.names()[1], "2");
  BOOST_CHECK_EQUAL(other.names()[2], "three");
  BOOST_CHECK_EQUAL(tested.names().size(), 0);
}

/**
 * @test Verify we can create jb::config_attribute objects with
 * complex constructors
 */
BOOST_AUTO_TEST_CASE(config_attribute_variadic_constructor) {
  test_variadic tested;

  BOOST_CHECK_EQUAL(tested.foo().first, 1);
  BOOST_CHECK_EQUAL(tested.foo().second, 2);
}

namespace {

class config0 : public jb::config_object {
public:
  config0()
      : x(desc("x"), this)
      , y(desc("y"), this)
      , z(desc("z"), this) {
  }

  config_object_constructors(config0);

  jb::config_attribute<config0, int> x;
  jb::config_attribute<config0, int> y;
  jb::config_attribute<config0, int> z;
};

// Convenience functions for the unit tests.
std::ostream& operator<<(std::ostream& os, config0 const& x) {
  return os << "{x=" << x.x() << ", y=" << x.y() << ", z=" << x.z() << "}";
}

bool operator==(config0 const& lhs, config0 const& rhs) {
  return lhs.x() == rhs.x() and lhs.y() == rhs.y() and lhs.z() == rhs.z();
}

config0 make_config0(int x, int y, int z) {
  return std::move(config0().x(x).y(y).z(z));
}

class config1 : public jb::config_object {
public:
  config1()
      : list(desc("list"), this, {"ini", "mini", "myni", "mo"})
      , pos(desc("pos", "config0"), this) {
  }

  config_object_constructors(config1);

  jb::config_attribute<config1, std::vector<std::string>> list;
  jb::config_attribute<config1, config0> pos;
};

} // namespace anonymous

/**
 * @test Verify the framework support deeply nested configs with
 * by_class overrides.
 */
BOOST_AUTO_TEST_CASE(config_object_apply) {
  char const contents[] = R"""(# YAML overrides
# This applies to all objects of type 'config0'
:config0:
  x: -1
  y: -2
  z: -3
list: ['1', '3', '5', '7']
pos:
  x: 2
  y: 4
)""";

  BOOST_TEST_MESSAGE("Applying overrides from\n" << contents << "\n");
  config1 tested;
  std::vector<std::string> expected({"ini", "mini", "myni", "mo"});
  BOOST_CHECK_EQUAL_COLLECTIONS(
      tested.list().begin(), tested.list().end(), expected.begin(),
      expected.end());
  BOOST_CHECK_EQUAL(tested.pos(), make_config0(0, 0, 0));

  std::istringstream is(contents);
  int argc = 0;
  tested.load_overrides(argc, nullptr, is);

  expected.assign({"1", "3", "5", "7"});
  BOOST_CHECK_EQUAL_COLLECTIONS(
      tested.list().begin(), tested.list().end(), expected.begin(),
      expected.end());
  BOOST_CHECK_EQUAL(tested.pos(), make_config0(2, 4, -3));
}

namespace {
class config2 : public jb::config_object {
public:
  config2()
      : vars(desc("vars"), this) {
  }

  config_object_constructors(config2);

  jb::config_attribute<config2, std::vector<config1>> vars;
};
} // namespace anonymous

/**
 * @test Verify the framework supports vectors of config objects.
 */
BOOST_AUTO_TEST_CASE(config_object_vector) {
  char const contents[] = R"""(# YAML overrides
vars:
  - list: ['1', '3', '5', '7']
    pos:
      x: 2
      y: 4
  - list: ['2', '4', '6', '8']
    pos:
      y: 1
      z: 3
  - list: ['11']
    pos:
      x: 1
      y: 2
      z: 3
)""";

  BOOST_TEST_MESSAGE("Applying overrides from\n" << contents << "\n");

  config2 tested;
  std::istringstream is(contents);
  int argc = 0;
  tested.load_overrides(argc, nullptr, is);

  BOOST_REQUIRE_EQUAL(tested.vars().size(), 3);

  std::vector<std::string> expected;

  expected.assign({"1", "3", "5", "7"});
  BOOST_CHECK_EQUAL_COLLECTIONS(
      tested.vars()[0].list().begin(), tested.vars()[0].list().end(),
      expected.begin(), expected.end());
  BOOST_CHECK_EQUAL(tested.vars()[0].pos().x(), 2);
  BOOST_CHECK_EQUAL(tested.vars()[0].pos().y(), 4);

  expected.assign({"2", "4", "6", "8"});
  BOOST_CHECK_EQUAL_COLLECTIONS(
      tested.vars()[1].list().begin(), tested.vars()[1].list().end(),
      expected.begin(), expected.end());
  BOOST_CHECK_EQUAL(tested.vars()[1].pos().y(), 1);
  BOOST_CHECK_EQUAL(tested.vars()[1].pos().z(), 3);

  expected.assign({"11"});
  BOOST_CHECK_EQUAL_COLLECTIONS(
      tested.vars()[2].list().begin(), tested.vars()[2].list().end(),
      expected.begin(), expected.end());
  BOOST_CHECK_EQUAL(tested.vars()[2].pos(), make_config0(1, 2, 3));
}

/**
 * @test Verify the framework supports vectors of config objects that
 * are empty.
 */
BOOST_AUTO_TEST_CASE(config_object_vector_empty) {
  config2 tested;
  std::istringstream is("");
  char argv0[] = "not_a_path";
  char argv1[] = "--vars.0.pos.x=2";
  char* argv[] = {argv0, argv1, nullptr};
  int argc = sizeof(argv) / sizeof(argv[0]) - 1;
  tested.load_overrides(argc, argv, is);

  BOOST_REQUIRE_EQUAL(tested.vars().size(), 1);
  BOOST_CHECK_EQUAL(tested.vars()[0].pos().x(), 2);

  std::ostringstream os;
  BOOST_CHECK_NO_THROW(os << tested);
  config2 empty;
  BOOST_CHECK_NO_THROW(os << empty);
}

namespace {
class config3 : public jb::config_object {
public:
  config3()
      : foo(desc("foo", "config0"), this)
      , bar(desc("bar", "config0"), this)
      , baz(desc("baz", "config0"), this) {
  }

  config_object_constructors(config3);

  jb::config_attribute<config3, config0> foo;
  jb::config_attribute<config3, config0> bar;
  jb::config_attribute<config3, config0> baz;
};

class config4 : public jb::config_object {
public:
  config4()
      : ini(desc("ini"), this)
      , mini(desc("mini"), this)
      , myni(desc("myni"), this) {
  }

  config_object_constructors(config4);

  jb::config_attribute<config4, config3> ini;
  jb::config_attribute<config4, config3> mini;
  jb::config_attribute<config4, config3> myni;
};

} // namespace anonymous

/**
 * @test Verify the framework supports configuring by class in nested structs.
 */
BOOST_AUTO_TEST_CASE(config_object_nested_by_class) {
  char const contents[] = R"""(# YAML overrides
# This override applies to all objects of type config0, wherever
# they are found...
:config0:
  x: -1
  y: -1
  z: -1
ini:
  # While this configuration only applies to the objects in this scope...
  :config0:
    x: -2
    y: -2
    z: -2
  bar:
    x: 1
    y: 2
mini:
  foo:
    x: 3
  bar:
    y: 4
myni:
  # Notice that we can override just a few fields too
  :config0:
    z: -3
  baz:
    z: 5
)""";

  BOOST_TEST_MESSAGE("Applying overrides from doc\n" << contents << "\n");

  config4 tested;
  std::istringstream is(contents);
  int argc = 0;
  tested.load_overrides(argc, nullptr, is);

  BOOST_CHECK_EQUAL(tested.ini().foo(), make_config0(-2, -2, -2));
  BOOST_CHECK_EQUAL(tested.ini().bar(), make_config0(1, 2, -2));
  BOOST_CHECK_EQUAL(tested.ini().baz(), make_config0(-2, -2, -2));

  BOOST_CHECK_EQUAL(tested.mini().foo(), make_config0(3, -1, -1));
  BOOST_CHECK_EQUAL(tested.mini().bar(), make_config0(-1, 4, -1));
  BOOST_CHECK_EQUAL(tested.mini().baz(), make_config0(-1, -1, -1));

  BOOST_CHECK_EQUAL(tested.myni().foo(), make_config0(-1, -1, -3));
  BOOST_CHECK_EQUAL(tested.myni().bar(), make_config0(-1, -1, -3));
  BOOST_CHECK_EQUAL(tested.myni().baz(), make_config0(-1, -1, 5));

  std::ostringstream os;
  os << tested;
}

/**
 * @test Verify that we can load configurations from an iostream.
 */
BOOST_AUTO_TEST_CASE(config_object_load) {
  char const contents[] = R"""(# YAML overrides
:config0:
  x: -1
  y: -2
  z: -3
list: ['1', '3', '5', '7']
pos:
  x: 2
  y: 4
)""";

  std::istringstream is(contents);

  config1 tested;
  char argv0[] = "not_a_path";
  char* argv[] = {argv0, nullptr};
  int argc = sizeof(argv) / sizeof(argv[0]) - 1;

  tested.load_overrides(argc, argv, is);

  std::vector<std::string> expected({"1", "3", "5", "7"});
  BOOST_CHECK_EQUAL_COLLECTIONS(
      tested.list().begin(), tested.list().end(), expected.begin(),
      expected.end());
  BOOST_CHECK_EQUAL(tested.pos(), make_config0(2, 4, -3));
}

/**
 * @test Verify that we apply command-line arguments after the overrides.
 */
BOOST_AUTO_TEST_CASE(config_object_cmdline_args) {
  char const contents[] = R"""(# YAML overrides
:config0:
  x: -1
  y: -2
  z: -3
pos:
  x: 2
  y: 4
)""";

  std::istringstream is(contents);

  config1 tested;
  char argv0[] = "not_a_path";
  char argv1[] = "--pos.x=3";
  char argv2[] = "--list=1";
  char argv3[] = "--list=3";
  char argv4[] = "--list=5";
  char argv5[] = "--list=7";
  char* argv[] = {argv0, argv1, argv2, argv3, argv4, argv5, nullptr};
  int argc = sizeof(argv) / sizeof(argv[0]) - 1;

  tested.load_overrides(argc, argv, is);

  std::vector<std::string> expected({"1", "3", "5", "7"});
  BOOST_CHECK_EQUAL_COLLECTIONS(
      tested.list().begin(), tested.list().end(), expected.begin(),
      expected.end());
  BOOST_CHECK_EQUAL(tested.pos(), make_config0(3, 4, -3));
}

/**
 * @test Verify that config objects raise usage exceptions.
 */
BOOST_AUTO_TEST_CASE(config_object_usage) {
  config1 tested;
  char argv0[] = "binary";
  char argv1[] = "--help";
  char* argv[] = {argv0, argv1, nullptr};
  int argc = sizeof(argv) / sizeof(argv[0]) - 1;
  std::istringstream is("");

  BOOST_CHECK_THROW(tested.load_overrides(argc, argv, is), jb::usage);
}

/**
 * @test Verify that config objects raise exceptions when presented
 * with invalid options.
 */
BOOST_AUTO_TEST_CASE(config_object_invalid_option) {
  config1 tested;
  char argv0[] = "binary";
  char argv1[] = "--invalid-option";
  char* argv[] = {argv0, argv1, nullptr};
  int argc = sizeof(argv) / sizeof(argv[0]) - 1;
  std::istringstream is("");

  BOOST_CHECK_THROW(tested.load_overrides(argc, argv, is), std::exception);
}

namespace {
class config5 : public jb::config_object {
public:
  config5()
      : foo(desc("foo"), this) {
  }

  config_object_constructors(config5);

  jb::config_attribute<config5, std::pair<int, int>> foo;
};
} // anonymous namespace

/**
 * @test Verify config_objects can handle std::pair<>
 */
BOOST_AUTO_TEST_CASE(config_object_pair_yaml) {
  char const contents[] = R"""(# YAML overrides
foo:
  - 2
  - 7
)""";

  std::istringstream is(contents);
  config5 tested;
  int argc = 0;
  tested.load_overrides(argc, nullptr, is);

  BOOST_CHECK_EQUAL(tested.foo().first, 2);
  BOOST_CHECK_EQUAL(tested.foo().second, 7);
}

/**
 * @test Verify config_objects can handle std::pair<>
 */
BOOST_AUTO_TEST_CASE(config_object_pair_options) {
  char const contents[] = R"""(# YAML overrides
foo:
  - 2
  - 7
)""";

  std::istringstream is(contents);
  config5 tested;
  char argv0[] = "binary";
  char argv1[] = "--foo.first=42";
  char argv2[] = "--foo.second=43";
  char* argv[] = {argv0, argv1, argv2, nullptr};
  int argc = sizeof(argv) / sizeof(argv[0]) - 1;
  tested.load_overrides(argc, argv, is);

  BOOST_CHECK_EQUAL(tested.foo().first, 42);
  BOOST_CHECK_EQUAL(tested.foo().second, 43);

  std::ostringstream os;
  BOOST_CHECK_NO_THROW(os << tested);
}

namespace {
class config6 : public jb::config_object {
public:
  config6()
      : foo(desc("foo"), this)
      , bar(desc("bar", "config0").help("not much help"), this)
      , baz(desc("baz", "config0").help("not much help"), this) {
  }

  config_object_constructors(config6);

  jb::config_attribute<config6, std::string> foo;
  jb::config_attribute<config6, config0> bar;
  jb::config_attribute<config6, config0> baz;
};
} // anonymous namespace

/**
 * @test Verify that config object works correctly with real files and
 * an environment variable.
 */
BOOST_AUTO_TEST_CASE(config_object_config_file_env) {
  char const contents[] = R"""(# YAML overrides
:config0:
  x: -1
  y: -2
  z: -3
foo: this is a long string
baz:
  z: 4
)""";
  namespace fs = boost::filesystem;
  fs::path tmpdir = fs::temp_directory_path() / fs::unique_path();
  BOOST_TEST_MESSAGE("creating unique tempdir at " << tmpdir);
  BOOST_REQUIRE(fs::create_directories(tmpdir));
  std::shared_ptr<int> delete_dir(new int(5), [tmpdir](int* x) {
    delete x;
    fs::remove_all(tmpdir);
  });
  std::string filename = "test.yml";
  // ... create a file in the temporary directory with these
  {
    fs::path base = fs::path(jb::sysconfdir()).filename();
    BOOST_REQUIRE(fs::create_directories(tmpdir / base));
    fs::path fullpath = tmpdir / base / filename;
    BOOST_TEST_MESSAGE("writing contents to " << fullpath.string());
    std::ofstream os(fullpath.string());
    os << contents;
  }

  // ... setup the environment variable to the test directory ...
  (void)setenv("TEST_ROOT", tmpdir.string().c_str(), true);
  char argv0[] = "binary";
  char argv1[] = "--bar.x=42";
  char argv2[] = "--baz.y=24";
  char* argv[] = {argv0, argv1, argv2, nullptr};
  int argc = sizeof(argv) / sizeof(argv[0]) - 1;
  config6 tested;
  tested.load_overrides(argc, argv, filename, "TEST_ROOT");
  BOOST_CHECK_EQUAL(tested.foo(), "this is a long string");
  BOOST_CHECK_EQUAL(tested.bar(), make_config0(42, -2, -3));
  BOOST_CHECK_EQUAL(tested.baz(), make_config0(-1, 24, 4));
}

/**
 * @test Verify that config object works correctly with a missing file
 * and the environment variable.
 */
BOOST_AUTO_TEST_CASE(config_object_config_file_missing_with_env) {
  namespace fs = boost::filesystem;
  fs::path tmpdir = fs::temp_directory_path() / fs::unique_path();
  BOOST_TEST_MESSAGE("creating unique tempdir at " << tmpdir);
  BOOST_REQUIRE(fs::create_directories(tmpdir));
  std::shared_ptr<int> delete_dir(new int(5), [tmpdir](int* x) {
    delete x;
    fs::remove_all(tmpdir);
  });
  std::string filename = "test.yml";

  // ... setup the environment variable to the test directory ...
  (void)setenv("TEST_ROOT", tmpdir.string().c_str(), true);
  char argv0[] = "binary";
  char argv1[] = "--bar.x=42";
  char argv2[] = "--baz.y=24";
  char* argv[] = {argv0, argv1, argv2, nullptr};
  int argc = sizeof(argv) / sizeof(argv[0]) - 1;
  config6 tested;
  tested.load_overrides(argc, argv, filename, "TEST_ROOT");
  BOOST_CHECK_EQUAL(tested.foo(), "");
  BOOST_CHECK_EQUAL(tested.bar(), make_config0(42, 0, 0));
  BOOST_CHECK_EQUAL(tested.baz(), make_config0(0, 24, 0));
}

/**
 * @test Verify that config object works correctly with real files and
 * the default environment variable.
 */
BOOST_AUTO_TEST_CASE(config_object_config_file) {
  char const contents[] = R"""(# YAML overrides
:config0:
  x: -1
  y: -2
  z: -3
foo: this is a long string
baz:
  z: 4
)""";
  namespace fs = boost::filesystem;
  fs::path tmpdir = fs::temp_directory_path() / fs::unique_path();
  BOOST_TEST_MESSAGE("creating unique tempdir at " << tmpdir);
  BOOST_REQUIRE(fs::create_directories(tmpdir));
  std::shared_ptr<int> delete_dir(new int(5), [tmpdir](int* x) {
    delete x;
    fs::remove_all(tmpdir);
  });
  std::string filename = "test.yml";
  // ... create a file in the temporary directory with these
  {
    fs::path base = fs::path(jb::sysconfdir()).filename();
    BOOST_REQUIRE(fs::create_directories(tmpdir / base));
    fs::path fullpath = tmpdir / base / filename;
    BOOST_TEST_MESSAGE("writing contents to " << fullpath.string());
    std::ofstream os(fullpath.string());
    os << contents;
  }

  // ... setup the environment variable to the test directory ...
  (void)setenv("JAYBEAMS_ROOT", tmpdir.string().c_str(), true);
  char argv0[] = "binary";
  char argv1[] = "--bar.x=42";
  char argv2[] = "--baz.y=24";
  char* argv[] = {argv0, argv1, argv2, nullptr};
  int argc = sizeof(argv) / sizeof(argv[0]) - 1;
  config6 tested;
  tested.load_overrides(argc, argv, filename);
  BOOST_CHECK_EQUAL(tested.foo(), "this is a long string");
  BOOST_CHECK_EQUAL(tested.bar(), make_config0(42, -2, -3));
  BOOST_CHECK_EQUAL(tested.baz(), make_config0(-1, 24, 4));
}

/**
 * @test Verify that config object works correctly when the real file
 * is not found using the default environment variable.
 */
BOOST_AUTO_TEST_CASE(config_object_config_file_missing) {
  std::string filename("missing-config-file.bad.bad.bad.yml");

  char argv0[] = "binary";
  char argv1[] = "--foo=this is a long string";
  char argv2[] = "--baz.y=24";
  char* argv[] = {argv0, argv1, argv2, nullptr};
  int argc = sizeof(argv) / sizeof(argv[0]) - 1;
  config6 tested;
  tested.load_overrides(argc, argv, filename);
  BOOST_CHECK_EQUAL(tested.foo(), "this is a long string");
  BOOST_CHECK_EQUAL(tested.bar(), make_config0(0, 0, 0));
  BOOST_CHECK_EQUAL(tested.baz(), make_config0(0, 24, 0));
}

namespace {
class config7 : public jb::config_object {
public:
  config7()
      : foo(desc("foo"), this)
      , bar(desc("bar").help("not much help").positional(), this)
      , baz(desc("baz").help("not much help").positional(), this) {
  }

  config_object_constructors(config7);

  jb::config_attribute<config7, std::string> foo;
  jb::config_attribute<config7, std::string> bar;
  jb::config_attribute<config7, std::string> baz;
};
} // anonymous namespace

/**
 * @test Verify that config object works correctly with real files and
 * an environment variable.
 */
BOOST_AUTO_TEST_CASE(config_object_positional) {
  std::istringstream is("");
  char argv0[] = "binary";
  char argv1[] = "should-be-bar";
  char argv2[] = "should-be-baz";
  char argv3[] = "--foo=another";
  char* argv[] = {argv0, argv1, argv2, argv3, nullptr};
  int argc = sizeof(argv) / sizeof(argv[0]) - 1;
  config7 tested;
  tested.load_overrides(argc, argv, is);
  BOOST_CHECK_EQUAL(tested.foo(), "another");
  BOOST_CHECK_EQUAL(tested.bar(), "should-be-bar");
  BOOST_CHECK_EQUAL(tested.baz(), "should-be-baz");
}

/**
 * @test Complete coverage for jb::usage
 */
BOOST_AUTO_TEST_CASE(usage_coverage) {
  jb::usage a("foo", 0);
  jb::usage b(std::string("foo"), 0);

  BOOST_CHECK_EQUAL(a.exit_status(), b.exit_status());
  BOOST_CHECK_EQUAL(a.what(), b.what());
}
