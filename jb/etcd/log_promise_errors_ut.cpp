#include "jb/etcd/log_promise_errors.hpp"

#include <boost/test/unit_test.hpp>

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

/**
 * @test Verify that jb::etcd::log_promise_errors generates good messages.
 */
BOOST_AUTO_TEST_CASE(check_log_promise_errors_text) {
  std::runtime_error ex("foobar");
  std::runtime_error future_ex("future foobar");

  auto actual = jb::etcd::log_promise_errors_text(
      std::make_exception_ptr(ex), std::make_exception_ptr(future_ex), "header",
      "test");
  BOOST_CHECK_EQUAL(
      actual, std::string("header: std::exception<future foobar> "
                          "raised by promise in test while setting the "
                          "promise to exception=std::exception<foobar>"));

  actual = jb::etcd::log_promise_errors_text(
      std::make_exception_ptr(int(42)), std::make_exception_ptr(future_ex),
      "header", "test");
  BOOST_CHECK_EQUAL(
      actual, std::string("header: std::exception<future foobar> "
                          "raised by promise in test while setting the "
                          "promise to exception=unknown exception"));

  actual = jb::etcd::log_promise_errors_text(
      std::make_exception_ptr(ex), std::make_exception_ptr(int(42)), "header",
      "test");
  BOOST_CHECK_EQUAL(
      actual, std::string("header: unknown exception "
                          "raised by promise in test while setting the "
                          "promise to exception=std::exception<foobar>"));

  // ... and these are just for code coverage ...
  auto noex = std::current_exception();

  actual = jb::etcd::log_promise_errors_text(
      std::make_exception_ptr(ex), noex, "header", "test");
  BOOST_CHECK_EQUAL(
      actual, std::string("header: no exception "
                          "raised by promise in test while setting the "
                          "promise to exception=std::exception<foobar>"));
}
