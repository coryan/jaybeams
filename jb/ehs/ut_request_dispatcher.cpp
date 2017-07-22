#include <jb/ehs/request_dispatcher.hpp>
#include <jb/strtonum.hpp>

#include <boost/test/unit_test.hpp>

/**
 * @test Verify that jb::ehs::request_dispatcher works as expected.
 */
BOOST_AUTO_TEST_CASE(request_dispatcher_base) {
  using namespace jb::ehs;
  request_dispatcher tested("test");
  request_type req;
  req.method(beast::http::verb::get);
  req.target("/");
  req.version = 11;
  req.insert("host", "example.com:80");
  req.insert("user-agent", "unit test");

  response_type res = tested.process(req);
  BOOST_CHECK_EQUAL(res.result_int(), (int)beast::http::status::not_found);
  BOOST_CHECK_EQUAL(res.version, 11);
  BOOST_CHECK_EQUAL(res["server"], "test");

  tested.add_handler("/", [](request_type const& req, response_type& res) {
    res.insert("content-type", "text/plain");
    res.body = "OK\r\n";
  });
  res = tested.process(req);
  BOOST_CHECK_EQUAL(res.result_int(), (int)beast::http::status::ok);
  BOOST_CHECK_EQUAL(res.version, 11);
  BOOST_CHECK_EQUAL(res["server"], "test");
  BOOST_CHECK_EQUAL(res.body, "OK\r\n");

  req.target("/not-there");
  res = tested.process(req);
  BOOST_CHECK_EQUAL(res.result_int(), (int)beast::http::status::not_found);
  BOOST_CHECK_EQUAL(res.version, 11);
  BOOST_CHECK_EQUAL(res["server"], "test");

  tested.add_handler(
      "/not-there", [](request_type const& req, response_type& res) {
        res.insert("content-type", "text/plain");
        res.body = "Fine I guess\r\n";
      });

  res = tested.process(req);
  BOOST_CHECK_EQUAL(res.result_int(), (int)beast::http::status::ok);
  BOOST_CHECK_EQUAL(res.version, 11);
  BOOST_CHECK_EQUAL(res["server"], "test");
  BOOST_CHECK_EQUAL(res.body, "Fine I guess\r\n");
}

/**
 * @test Verify that jb::ehs::request_dispatcher works as expected for errors.
 */
BOOST_AUTO_TEST_CASE(request_dispatcher_error) {
  using namespace jb::ehs;
  request_dispatcher tested("test");
  auto thrower = [](request_type const& req, response_type& res) {
    throw std::runtime_error("bad stuff happens");
  };
  tested.add_handler("/error", thrower);

  request_type req;
  req.method(beast::http::verb::get);
  req.target("/error");
  req.version = 11;
  req.insert("host", "example.com:80");
  req.insert("user-agent", "unit test");

  response_type res = tested.process(req);
  BOOST_CHECK_EQUAL(
      res.result_int(), (int)beast::http::status::internal_server_error);
  BOOST_CHECK_EQUAL(res.version, 11);
  BOOST_CHECK_EQUAL(res["server"], "test");

  BOOST_CHECK_THROW(tested.add_handler("/error", thrower), std::runtime_error);
  BOOST_CHECK_EQUAL(tested.get_write_500(), 1);
}

/**
 * @test Verify that jb::ehs::request_dispatcher counters work as expected.
 */
BOOST_AUTO_TEST_CASE(request_dispatcher_counter) {
  using namespace jb::ehs;
  request_dispatcher tested("test");
  tested.add_handler("/path", [](request_type const& req, response_type& res) {
    if (req.count("x-return-status") > 0) {
      std::string val(req["x-return-status"]);
      int r;
      (void)jb::strtonum(val, r);
      res.result(r);
    } else {
      BOOST_CHECK_MESSAGE(false, "x-return-status not set");
    }
    res.insert("content-type", "text/plain");
    res.body = "OK\r\n";
  });

  request_type req;
  req.method(beast::http::verb::get);
  req.target("/path");
  req.version = 11;
  req.insert("host", "example.com:80");
  req.insert("user-agent", "unit test");

  req.insert("x-return-status", "200");
  response_type res = tested.process(req);
  BOOST_CHECK_EQUAL(res.result_int(), 200);
  BOOST_CHECK_EQUAL(tested.get_write_200(), 1);

  req.set("x-return-status", "100");
  res = tested.process(req);
  BOOST_CHECK_EQUAL(res.result_int(), 100);
  BOOST_CHECK_EQUAL(tested.get_write_100(), 1);

  req.set("x-return-status", "204");
  res = tested.process(req);
  BOOST_CHECK_EQUAL(res.result_int(), 204);
  BOOST_CHECK_EQUAL(tested.get_write_200(), 2);

  req.set("x-return-status", "300");
  res = tested.process(req);
  BOOST_CHECK_EQUAL(res.result_int(), 300);
  BOOST_CHECK_EQUAL(tested.get_write_300(), 1);

  req.set("x-return-status", "400");
  res = tested.process(req);
  BOOST_CHECK_EQUAL(res.result_int(), 400);
  BOOST_CHECK_EQUAL(tested.get_write_400(), 1);

  req.set("x-return-status", "500");
  res = tested.process(req);
  BOOST_CHECK_EQUAL(res.result_int(), 500);
  BOOST_CHECK_EQUAL(tested.get_write_500(), 1);

  req.set("x-return-status", "600");
  res = tested.process(req);
  BOOST_CHECK_EQUAL(res.result_int(), 600);
  BOOST_CHECK_EQUAL(tested.get_write_invalid(), 1);

  // ... verify that the counters were only updated for the right
  // event ...
  BOOST_CHECK_EQUAL(tested.get_write_100(), 1);
  BOOST_CHECK_EQUAL(tested.get_write_200(), 2);
  BOOST_CHECK_EQUAL(tested.get_write_300(), 1);
  BOOST_CHECK_EQUAL(tested.get_write_400(), 1);
  BOOST_CHECK_EQUAL(tested.get_write_500(), 1);
}

/**
 * @test Verify that jb::ehs::request_dispatcher counters work as expected.
 */
BOOST_AUTO_TEST_CASE(request_dispatcher_network_counter) {
  using namespace jb::ehs;
  request_dispatcher tested("test");
  BOOST_CHECK_EQUAL(tested.get_accept_error(), 0);
  tested.count_accept_error();
  BOOST_CHECK_EQUAL(tested.get_accept_error(), 1);

  BOOST_CHECK_EQUAL(tested.get_accept_ok(), 0);
  tested.count_accept_ok();
  BOOST_CHECK_EQUAL(tested.get_accept_ok(), 1);

  BOOST_CHECK_EQUAL(tested.get_write_error(), 0);
  tested.count_write_error();
  BOOST_CHECK_EQUAL(tested.get_write_error(), 1);

  BOOST_CHECK_EQUAL(tested.get_write_ok(), 0);
  tested.count_write_ok();
  BOOST_CHECK_EQUAL(tested.get_write_ok(), 1);

  BOOST_CHECK_EQUAL(tested.get_read_error(), 0);
  tested.count_read_error();
  BOOST_CHECK_EQUAL(tested.get_read_error(), 1);

  BOOST_CHECK_EQUAL(tested.get_read_ok(), 0);
  tested.count_read_ok();
  BOOST_CHECK_EQUAL(tested.get_read_ok(), 1);

  BOOST_CHECK_EQUAL(tested.get_open_connection(), 0);
  tested.count_open_connection();
  BOOST_CHECK_EQUAL(tested.get_open_connection(), 1);

  BOOST_CHECK_EQUAL(tested.get_close_connection(), 0);
  tested.count_close_connection();
  BOOST_CHECK_EQUAL(tested.get_close_connection(), 1);

  // ... verify that no counters get accidentally updated by other
  // calls ...
  BOOST_CHECK_EQUAL(tested.get_open_connection(), 1);
  BOOST_CHECK_EQUAL(tested.get_close_connection(), 1);
  BOOST_CHECK_EQUAL(tested.get_read_ok(), 1);
  BOOST_CHECK_EQUAL(tested.get_read_error(), 1);
  BOOST_CHECK_EQUAL(tested.get_write_ok(), 1);
  BOOST_CHECK_EQUAL(tested.get_write_error(), 1);
  BOOST_CHECK_EQUAL(tested.get_accept_ok(), 1);
  BOOST_CHECK_EQUAL(tested.get_accept_error(), 1);
}

/**
 * @test Verify that jb::ehs::request_dispatcher::append_metrics works
 * as expected
 */
BOOST_AUTO_TEST_CASE(request_dispatcher_append_metrics) {
  using namespace jb::ehs;
  request_dispatcher tested("test");

  response_type res;
  tested.append_metrics(res);
  BOOST_CHECK_NE(res.body, "");
}
