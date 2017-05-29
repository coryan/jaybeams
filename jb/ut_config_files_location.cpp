#include <jb/config_files_location.hpp>

#include <skye/mock_function.hpp>

#include <boost/filesystem/operations.hpp>
#include <boost/test/unit_test.hpp>

namespace fs = boost::filesystem;

/**
 * Mocks and helper types used in the jb::config_files_location tests.
 */
namespace {
template <typename Functor>
struct shared_functor {
  shared_functor()
      : functor_(new Functor) {
  }

  Functor* operator->() {
    return functor_.get();
  }

  template <typename... T>
  auto operator()(T&&... a) -> decltype(Functor()(std::forward<T>(a)...)) {
    return (*functor_)(std::forward<T>(a)...);
  }

  std::shared_ptr<Functor> functor_;
};

/// Mock implementation for getenv()
typedef shared_functor<skye::mock_function<char const*(char const*)>>
    mock_getenv;

/// Mock implementation for the validator()
typedef shared_functor<skye::mock_function<bool(fs::path const&)>>
    mock_validator;

/// A convenience predicate to feed into whenp()
struct s_eq {
  explicit s_eq(char const* s)
      : match(s) {
  }
  bool operator()(char const* t) const {
    return match == t;
  }
  std::string match;
};

typedef jb::config_files_locations<mock_getenv, mock_validator> mocked;

struct trivial_getenv {
  char const* operator()(char const*) {
    return nullptr;
  }
};

struct trivial_validator {
  bool operator()(fs::path const&) {
    return true;
  }
};

typedef jb::config_files_locations<trivial_getenv, trivial_validator> trivial;
} // anonymous namespace

/**
 * @test Verify that the common constructors compile.
 */
BOOST_AUTO_TEST_CASE(config_files_location_constructors) {
  trivial_getenv getenv;

  trivial t0(fs::path("/foo/var/baz/program"), "TEST_ROOT", getenv);
  BOOST_CHECK(not t0.search_path().empty());

  trivial t1(fs::path("/foo/var/baz/program"), "TEST_ROOT");
  BOOST_CHECK(not t1.search_path().empty());

  trivial t3(fs::path("/foo/var/baz/program"), getenv);
  BOOST_CHECK(not t3.search_path().empty());

  trivial t4(fs::path("/foo/var/baz/program"));
  BOOST_CHECK(not t4.search_path().empty());

  trivial t5("/foo/var/baz/program", "TEST_ROOT", getenv);
  BOOST_CHECK(not t5.search_path().empty());

  trivial t6("/foo/var/baz/program", "TEST_ROOT");
  BOOST_CHECK(not t6.search_path().empty());

  trivial t7("/foo/var/baz/program", getenv);
  BOOST_CHECK(not t7.search_path().empty());

  trivial t8("/foo/var/baz/program");
  BOOST_CHECK(not t8.search_path().empty());
}

/**
 * @test Verify that a simple config_files_locations<> works as expected.
 */
BOOST_AUTO_TEST_CASE(config_files_location_program_root) {
  mock_validator validator;
  mock_getenv getenv;

  getenv->clear();
  getenv->whenp(s_eq("TEST_ROOT")).returns("/test/path");
  getenv->whenp(s_eq("JAYBEAMS_ROOT")).returns("/install/path");
  validator->returns(true);

  fs::path programdir = fs::path("/foo/var/baz");
  mocked t(programdir / "program", "TEST_ROOT", getenv);

  fs::path etc = fs::path(jb::sysconfdir()).filename();
  std::vector<fs::path> expected(
      {"/test/path" / etc, "/install/path" / etc, jb::sysconfdir(),
       programdir});

  BOOST_CHECK_EQUAL_COLLECTIONS(
      t.search_path().begin(), t.search_path().end(), expected.begin(),
      expected.end());
}

/**
 * @test Verify that a simple config_files_locations<> works without a
 * program root.
 */
BOOST_AUTO_TEST_CASE(config_files_location_no_program_root) {
  mock_validator validator;
  mock_getenv getenv;

  getenv->clear();
  getenv->whenp(s_eq("TEST_ROOT")).returns("/test/path");
  getenv->whenp(s_eq("JAYBEAMS_ROOT")).returns("/install/path");
  validator->returns(true);

  fs::path programdir = fs::path("/foo/var/baz");
  mocked t(programdir / "program", getenv);

  fs::path etc = fs::path(jb::sysconfdir()).filename();
  std::vector<fs::path> expected(
      {"/install/path" / etc, jb::sysconfdir(), programdir});
  BOOST_CHECK_EQUAL_COLLECTIONS(
      t.search_path().begin(), t.search_path().end(), expected.begin(),
      expected.end());
}

/**
 * @test Verify that a simple config_files_locations<> works with some
 * variables undefined.
 */
BOOST_AUTO_TEST_CASE(config_files_location_undefined_undef_test_root) {
  mock_validator validator;
  mock_getenv getenv;

  getenv->clear();
  getenv->whenp(s_eq("TEST_ROOT")).returns(nullptr);
  getenv->whenp(s_eq("JAYBEAMS_ROOT")).returns("/install/path");
  validator->returns(true);

  fs::path programdir = fs::path("/foo/var/baz");
  mocked t(programdir / "program", "TEST_ROOT", getenv);

  fs::path etc = fs::path(jb::sysconfdir()).filename();
  std::vector<fs::path> expected(
      {"/install/path" / etc, jb::sysconfdir(), programdir});

  BOOST_CHECK_EQUAL_COLLECTIONS(
      t.search_path().begin(), t.search_path().end(), expected.begin(),
      expected.end());
}

/**
 * @test Verify that a simple config_files_locations<> works with some
 * variables undefined.
 */
BOOST_AUTO_TEST_CASE(config_files_location_undefined_undef_system_root) {
  mock_validator validator;
  mock_getenv getenv;

  getenv->clear();
  getenv->whenp(s_eq("TEST_ROOT")).returns("/test/path");
  getenv->whenp(s_eq("JAYBEAMS_ROOT")).returns(nullptr);
  validator->returns(true);

  fs::path programdir = fs::path("/foo/var/baz");
  mocked t(programdir / "program", "TEST_ROOT", getenv);

  fs::path etc = fs::path(jb::sysconfdir()).filename();
  std::vector<fs::path> expected(
      {"/test/path" / etc, jb::sysconfdir(), programdir});

  BOOST_CHECK_EQUAL_COLLECTIONS(
      t.search_path().begin(), t.search_path().end(), expected.begin(),
      expected.end());
}

/**
 * @test Verify that a simple config with a valid path for the binary works.
 */
BOOST_AUTO_TEST_CASE(config_files_location_installed_binary) {
  mock_validator validator;
  mock_getenv getenv;

  getenv->clear();
  getenv->whenp(s_eq("TEST_ROOT")).returns("/test/path");
  getenv->whenp(s_eq("JAYBEAMS_ROOT")).returns("/install/path");
  validator->returns(true);

  fs::path etc = fs::path(jb::sysconfdir()).filename();

  fs::path install_path = fs::path("/install") / jb::bindir();
  fs::path program = install_path / "program";

  mocked t(program, "TEST_ROOT", getenv);

  std::vector<fs::path> expected(
      {"/test/path" / etc, "/install/path" / etc, jb::sysconfdir(),
       program.parent_path().parent_path() / etc});

  BOOST_CHECK_EQUAL_COLLECTIONS(
      t.search_path().begin(), t.search_path().end(), expected.begin(),
      expected.end());
}

/**
 * @test Verify that a simple config_files_locations<> works when the
 * program is not path.
 */
BOOST_AUTO_TEST_CASE(config_files_location_no_program_path) {
  mock_validator validator;
  mock_getenv getenv;

  getenv->clear();
  getenv->whenp(s_eq("TEST_ROOT")).returns("/test/path");
  getenv->whenp(s_eq("JAYBEAMS_ROOT")).returns("/install/path");
  validator->returns(true);

  mocked t("program", getenv);

  fs::path etc = fs::path(jb::sysconfdir()).filename();
  std::vector<fs::path> expected({"/install/path" / etc, jb::sysconfdir()});
  BOOST_CHECK_EQUAL_COLLECTIONS(
      t.search_path().begin(), t.search_path().end(), expected.begin(),
      expected.end());
}

/**
 * @test Verify that the search algorithm works as expected.
 */
BOOST_AUTO_TEST_CASE(config_files_location_find) {
  mock_getenv getenv;

  getenv->clear();
  getenv->whenp(s_eq("TEST_ROOT")).returns("/test/path");
  getenv->whenp(s_eq("JAYBEAMS_ROOT")).returns("/install/path");

  fs::path etc = fs::path(jb::sysconfdir()).filename();

  fs::path install_path = fs::path("/install") / jb::bindir();
  fs::path program = install_path / "program";

  mocked t(program, "TEST_ROOT", getenv);

  // Fist check that the right exception is raised if no file can be found ...
  std::string filename = "test.yaml";
  mock_validator validator;
  validator->clear();
  validator->returns(false);

  BOOST_CHECK_THROW(
      t.find_configuration_file(filename, validator), std::runtime_error);

  // ... then check that each path is checked in order ...
  int n = 0;
  for (auto path : t.search_path()) {
    int cnt = 0;
    validator->action([&cnt, n]() { return cnt++ >= n; });
    auto full = path / filename;
    BOOST_CHECK_EQUAL(full, t.find_configuration_file(filename, validator));
    n++;
  }
}
