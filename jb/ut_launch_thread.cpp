#include <jb/launch_thread.hpp>

#include <boost/test/unit_test.hpp>
#include <mutex>
#include <thread>

/**
 * Helper classes to test jb::launch_thread.
 */
namespace {

class fixture {
public:
  fixture()
      : mu()
      , id()
      , value()
      , msg() {
  }

  void run(int x, std::string const& y) {
    std::unique_lock<std::mutex> lk(mu);
    id = std::this_thread::get_id();
    value = x;
    msg = y;
    std::this_thread::sleep_for(std::chrono::seconds(1));
  }

  void run(int x) {
    std::unique_lock<std::mutex> lk(mu);
    id = std::this_thread::get_id();
    value = x;
    msg = "no msg";
    std::this_thread::sleep_for(std::chrono::seconds(1));
  }

  int run_simple(int x, std::string const& y) {
    run(x, y);
    return 0;
  }

  std::mutex mu;
  std::thread::id id;
  int value;
  std::string msg;
};

fixture g;

void test_with_g(int x, std::string const& y) {
  g.run(x, y);
}

} // anonymous namespace

/**
 * @test Verify that jb::launch_thread<> compiles and works.
 */
BOOST_AUTO_TEST_CASE(launch_thread_basic) {
  jb::thread_config cfg;
  cfg.name("test-thread");

  std::thread t;
  jb::launch_thread(t, cfg, test_with_g, 42, "42");

  std::thread t0;
  auto f0 = std::make_shared<fixture>();
  auto fu0 = [f0](int x, std::string const& y) { f0->run(x, y); };

  jb::launch_thread(t0, cfg, fu0, 47, std::string("47"));

  std::thread t1;
  fixture f1;
  jb::launch_thread(t1, cfg, &fixture::run_simple, &f1, 1, "t1");

  t1.join();
  BOOST_CHECK_NE(f1.id, std::thread::id());
  BOOST_CHECK_EQUAL(f1.value, 1);
  BOOST_CHECK_EQUAL(f1.msg, "t1");

  t0.join();
  BOOST_CHECK_NE(f0->id, std::thread::id());
  BOOST_CHECK_EQUAL(f0->value, 47);
  BOOST_CHECK_EQUAL(f0->msg, "47");

  t.join();
  BOOST_CHECK_NE(g.id, std::thread::id());
  BOOST_CHECK_EQUAL(g.value, 42);
  BOOST_CHECK_EQUAL(g.msg, "42");
}

/**
 * @test Verify that jb::launch_thread<> detects OS errors.
 */
BOOST_AUTO_TEST_CASE(launch_thread_errors) {
  jb::thread_config cfg;
  cfg.ignore_setup_errors(false);
  cfg.name("name_too_long_should_fail__1234567890ABCDEF__");

  BOOST_TEST_MESSAGE("main id=" << std::this_thread::get_id());
  int cnt = 0;
  std::thread t;
  jb::launch_thread(t, cfg, [&cnt]() {
    BOOST_TEST_MESSAGE("id=" << std::this_thread::get_id());
    ++cnt;
  });
  t.join();
  BOOST_CHECK_EQUAL(cnt, 0);

  cfg.name("").affinity(jb::cpu_set::parse("512"));
  jb::launch_thread(t, cfg, [&cnt]() {
    BOOST_TEST_MESSAGE("id=" << std::this_thread::get_id());
    ++cnt;
  });
  t.join();
  BOOST_CHECK_EQUAL(cnt, 0);

  cfg.affinity(jb::cpu_set::parse("")).priority("1000000");
  jb::launch_thread(t, cfg, [&cnt]() {
    BOOST_TEST_MESSAGE("id=" << std::this_thread::get_id());
    ++cnt;
  });
  t.join();
  BOOST_CHECK_EQUAL(cnt, 0);
}

/**
 * @test Improve coverage, handle unknown exceptions.
 */
BOOST_AUTO_TEST_CASE(launch_thread_unknown_exception) {
  jb::thread_config cfg;

  BOOST_TEST_MESSAGE("main id=" << std::this_thread::get_id());
  int cnt = 0;
  std::thread t;
  jb::launch_thread(t, cfg, [&cnt]() {
    BOOST_TEST_MESSAGE("id=" << std::this_thread::get_id());
    ++cnt;
    throw "not a std::exception";
  });
  t.join();
}
