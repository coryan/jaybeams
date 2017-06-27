#include "jb/etcd/log_promise_errors.hpp"

#include <boost/test/unit_test.hpp>

/**
 * @test Verify that jb::etcd::log_promise_errors generates good messages.
 */
BOOST_AUTO_TEST_CASE(check_log_promise_errors_basic) {
  std::promise<bool> done;
  done.set_value(true);

  // ... setting an exception after setting a value raises an
  // exception, we want to log that, but reduce the amount of code
  // required to do so ...
  try {
    std::runtime_error ex("foobar");
    done.set_exception(std::make_exception_ptr(ex));
  } catch (...) {
    auto actual = jb::etcd::log_promise_errors_text(
        std::current_exception(), "header", "where");
    BOOST_TEST_MESSAGE(actual);
  }
}

/**
 * @test Verify that jb::etcd::log_promise_errors does not throw.
 */
BOOST_AUTO_TEST_CASE(check_log_promise_errors_no_throw) {

  std::runtime_error ex("foobar");
  std::promise<bool> empty;
  BOOST_CHECK_NO_THROW(jb::etcd::log_promise_errors(
      empty, std::make_exception_ptr(ex), "header", "test for empty"));

  std::promise<bool> full;
  full.set_value(false);
  BOOST_CHECK_NO_THROW(jb::etcd::log_promise_errors(
      full, std::make_exception_ptr(ex), "header", "test for full"));
}
