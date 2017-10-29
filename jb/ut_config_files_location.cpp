#include <jb/config_files_location.hpp>
#include <jb/gmock/init.hpp>

#include <boost/filesystem/operations.hpp>
#include <boost/test/unit_test.hpp>

namespace fs = boost::filesystem;

/**
 * Mocks and helper types used in the jb::config_files_location tests.
 */
namespace {
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

namespace {
template <typename Functor>
struct shared_functor {
  shared_functor()
      : mock(new Functor) {
  }

  template <typename... T>
  auto operator()(T&&... a) -> decltype(Functor().exec(std::forward<T>(a)...)) {
    return mock->exec(std::forward<T>(a)...);
  }

  void reset() {
    mock = std::shared_ptr<Functor>(new Functor);
  }

  std::shared_ptr<Functor> mock;
};

/// Mock implementation for getenv()
struct mock_getenv_f {
  MOCK_CONST_METHOD1(exec, char const*(char const*));
};
using mock_getenv = shared_functor<mock_getenv_f>;

/// Mock implementation for the validator()
struct mock_validator_f {
  MOCK_CONST_METHOD1(exec, bool(fs::path const&));
};
using mock_validator = shared_functor<mock_validator_f>;

/// The object under test
using mocked = jb::config_files_locations<mock_getenv, mock_validator>;

/// Configure mocks for most tests
void set_mocks(mock_getenv& getenv, mock_validator& validator,
               char const* test_root, char const* jaybeams_root,
               bool valid) {
  getenv.reset();
  using namespace ::testing;
  EXPECT_CALL(*getenv.mock, exec(Truly([](auto arg) {
                return std::string("TEST_ROOT") == arg;
              })))
      .WillRepeatedly(Return(test_root));
  EXPECT_CALL(*getenv.mock, exec(Truly([](auto arg) {
                return std::string("JAYBEAMS_ROOT") == arg;
              })))
      .WillRepeatedly(Return(jaybeams_root));

  validator.reset();
  EXPECT_CALL(*validator.mock, exec(_)).WillRepeatedly(Return(valid));
}
} // anonymous namespace

/**
 * @test Verify that a simple config_files_locations<> works as expected.
 */
BOOST_AUTO_TEST_CASE(config_files_location_program_root) {
  mock_getenv getenv;
  mock_validator validator;
  set_mocks(getenv, validator, "/test/path", "/install/path", true);
  
  fs::path programdir = fs::path("/foo/var/baz");
  mocked t(programdir / "program", "TEST_ROOT", getenv);

  fs::path etc = fs::path(jb::sysconfdir()).filename();
  std::vector<fs::path> expected({"/test/path" / etc, "/install/path" / etc,
                                  jb::sysconfdir(), programdir});
  using namespace ::testing;
  BOOST_CHECK_EQUAL_COLLECTIONS(
      t.search_path().begin(), t.search_path().end(), expected.begin(),
      expected.end());
}

/**
 * @test Verify that a simple config_files_locations<> works without a
 * program root.
 */
BOOST_AUTO_TEST_CASE(config_files_location_no_program_root) {
  mock_getenv getenv;
  mock_validator validator;
  set_mocks(getenv, validator, "/test/path", "/install/path", true);

  fs::path programdir = fs::path("/foo/var/baz");
  mocked t(programdir / "program", getenv);

  fs::path etc = fs::path(jb::sysconfdir()).filename();
  std::vector<fs::path> expected(
      {"/install/path" / etc, jb::sysconfdir(), programdir});
  using namespace ::testing;
  BOOST_CHECK_EQUAL_COLLECTIONS(
      t.search_path().begin(), t.search_path().end(), expected.begin(),
      expected.end());
}

/**
 * @test Verify that a simple config_files_locations<> works with some
 * variables undefined.
 */
BOOST_AUTO_TEST_CASE(config_files_location_undefined_undef_test_root) {
  mock_getenv getenv;
  mock_validator validator;
  set_mocks(getenv, validator, nullptr, "/install/path", true);

  fs::path programdir = fs::path("/foo/var/baz");
  mocked t(programdir / "program", "TEST_ROOT", getenv);

  fs::path etc = fs::path(jb::sysconfdir()).filename();
  std::vector<fs::path> expected(
      {"/install/path" / etc, jb::sysconfdir(), programdir});
  using namespace ::testing;
  BOOST_CHECK_EQUAL_COLLECTIONS(
      t.search_path().begin(), t.search_path().end(), expected.begin(),
      expected.end());
}

/**
 * @test Verify that a simple config_files_locations<> works with some
 * variables undefined.
 */
BOOST_AUTO_TEST_CASE(config_files_location_undefined_undef_system_root) {
  mock_getenv getenv;
  mock_validator validator;
  set_mocks(getenv, validator, "test/path", nullptr, true);

  fs::path programdir = fs::path("/foo/var/baz");
  mocked t(programdir / "program", "TEST_ROOT", getenv);

  fs::path etc = fs::path(jb::sysconfdir()).filename();
  std::vector<fs::path> expected(
      {"/test/path" / etc, jb::sysconfdir(), programdir});
  using namespace ::testing;
  BOOST_CHECK_EQUAL_COLLECTIONS(
      t.search_path().begin(), t.search_path().end(), expected.begin(),
      expected.end());
}

/**
 * @test Verify that a simple config with a valid path for the binary works.
 */
BOOST_AUTO_TEST_CASE(config_files_location_installed_binary) {
  mock_getenv getenv;
  mock_validator validator;
  set_mocks(getenv, validator, "/test/path", "/install/path", true);

  fs::path etc = fs::path(jb::sysconfdir()).filename();

  fs::path install_path = fs::path("/install") / jb::bindir();
  fs::path program = install_path / "program";

  mocked t(program, "TEST_ROOT", getenv);

  std::vector<fs::path> expected({"/test/path" / etc, "/install/path" / etc,
                                  jb::sysconfdir(),
                                  program.parent_path().parent_path() / etc});
  using namespace ::testing;
  BOOST_CHECK_EQUAL_COLLECTIONS(
      t.search_path().begin(), t.search_path().end(), expected.begin(),
      expected.end());
}

/**
 * @test Verify that a simple config_files_locations<> works when the
 * program is not path.
 */
BOOST_AUTO_TEST_CASE(config_files_location_no_program_path) {
  mock_getenv getenv;
  mock_validator validator;
  set_mocks(getenv, validator, "/test/path", "/install/path", true);

  mocked t("program", getenv);

  fs::path etc = fs::path(jb::sysconfdir()).filename();
  std::vector<fs::path> expected({"/install/path" / etc, jb::sysconfdir()});
  using namespace ::testing;
  BOOST_CHECK_EQUAL_COLLECTIONS(
      t.search_path().begin(), t.search_path().end(), expected.begin(),
      expected.end());
}

/**
 * @test Verify that the search algorithm works as expected.
 */
BOOST_AUTO_TEST_CASE(config_files_location_find) {
  mock_getenv getenv;
  mock_validator validator;
  set_mocks(getenv, validator, "/test/path", "/install/path", true);

  fs::path etc = fs::path(jb::sysconfdir()).filename();

  fs::path install_path = fs::path("/install") / jb::bindir();
  fs::path program = install_path / "program";

  mocked t(program, "TEST_ROOT", getenv);

  // Fist check that the right exception is raised if no file can be found ...
  std::string filename = "test.yaml";
  using namespace ::testing;
  validator.reset();
  EXPECT_CALL(*validator.mock, exec(_)).WillRepeatedly(Return(false));

  BOOST_CHECK_THROW(
      t.find_configuration_file(filename, validator), std::runtime_error);

  // ... then check that each path is checked in order ...
  for (auto path : t.search_path()) {
    validator.reset();
    EXPECT_CALL(*validator.mock, exec(Truly([path](auto arg) {
            return path == arg;
          }))).WillOnce(Return(true));
    auto full = path / filename;
    BOOST_CHECK_EQUAL(full, t.find_configuration_file(filename, validator));
  }
}
