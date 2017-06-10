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
  req.method = "GET";
  req.url = "/";
  req.version = 11;
  req.fields.insert("host", "example.com:80");
  req.fields.insert("user-agent", "unit test");

  response_type res = tested.process(req);
  BOOST_CHECK_EQUAL(res.status, 404);
  BOOST_CHECK_EQUAL(res.version, 11);
  BOOST_CHECK_EQUAL(res.fields["server"], "test");

  tested.add_handler("/", [](request_type const& req, response_type& res) {
    res.fields.insert("content-type", "text/plain");
    res.body = "OK\r\n";
  });
  res = tested.process(req);
  BOOST_CHECK_EQUAL(res.status, 200);
  BOOST_CHECK_EQUAL(res.version, 11);
  BOOST_CHECK_EQUAL(res.fields["server"], "test");
  BOOST_CHECK_EQUAL(res.body, "OK\r\n");

  req.url = "/not-there";
  res = tested.process(req);
  BOOST_CHECK_EQUAL(res.status, 404);
  BOOST_CHECK_EQUAL(res.version, 11);
  BOOST_CHECK_EQUAL(res.fields["server"], "test");

  tested.add_handler(
      "/not-there", [](request_type const& req, response_type& res) {
        res.fields.insert("content-type", "text/plain");
        res.body = "Fine I guess\r\n";
      });

  res = tested.process(req);
  BOOST_CHECK_EQUAL(res.status, 200);
  BOOST_CHECK_EQUAL(res.version, 11);
  BOOST_CHECK_EQUAL(res.fields["server"], "test");
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
  req.method = "GET";
  req.url = "/error";
  req.version = 11;
  req.fields.insert("host", "example.com:80");
  req.fields.insert("user-agent", "unit test");

  response_type res = tested.process(req);
  BOOST_CHECK_EQUAL(res.status, 500);
  BOOST_CHECK_EQUAL(res.version, 11);
  BOOST_CHECK_EQUAL(res.fields["server"], "test");

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
    if (req.fields.exists("x-return-status")) {
      std::string val = req.fields["x-return-status"].data();
      (void)jb::strtonum(val, res.status);
    }
    res.fields.insert("content-type", "text/plain");
    res.body = "OK\r\n";
  });

  request_type req;
  req.method = "GET";
  req.url = "/path";
  req.version = 11;
  req.fields.insert("host", "example.com:80");
  req.fields.insert("user-agent", "unit test");

  req.fields.replace("x-return-status", -10);
  response_type res = tested.process(req);
  BOOST_CHECK_EQUAL(res.status, -10);
  BOOST_CHECK_EQUAL(tested.get_write_invalid(), 1);

  req.fields.replace("x-return-status", 0);
  res = tested.process(req);
  BOOST_CHECK_EQUAL(res.status, 0);
  BOOST_CHECK_EQUAL(tested.get_write_invalid(), 2);

  req.fields.replace("x-return-status", 100);
  res = tested.process(req);
  BOOST_CHECK_EQUAL(res.status, 100);
  BOOST_CHECK_EQUAL(tested.get_write_100(), 1);

  req.fields.replace("x-return-status", 200);
  res = tested.process(req);
  BOOST_CHECK_EQUAL(res.status, 200);
  BOOST_CHECK_EQUAL(tested.get_write_200(), 1);

  req.fields.replace("x-return-status", 300);
  res = tested.process(req);
  BOOST_CHECK_EQUAL(res.status, 300);
  BOOST_CHECK_EQUAL(tested.get_write_300(), 1);

  req.fields.replace("x-return-status", 400);
  res = tested.process(req);
  BOOST_CHECK_EQUAL(res.status, 400);
  BOOST_CHECK_EQUAL(tested.get_write_400(), 1);

  req.fields.replace("x-return-status", 500);
  res = tested.process(req);
  BOOST_CHECK_EQUAL(res.status, 500);
  BOOST_CHECK_EQUAL(tested.get_write_500(), 1);

  req.fields.replace("x-return-status", 600);
  res = tested.process(req);
  BOOST_CHECK_EQUAL(res.status, 600);
  BOOST_CHECK_EQUAL(tested.get_write_invalid(), 3);

  // ... verify that the counters were only updated for the right
  // event ...
  BOOST_CHECK_EQUAL(tested.get_write_100(), 1);
  BOOST_CHECK_EQUAL(tested.get_write_200(), 1);
  BOOST_CHECK_EQUAL(tested.get_write_300(), 1);
  BOOST_CHECK_EQUAL(tested.get_write_400(), 1);
  BOOST_CHECK_EQUAL(tested.get_write_500(), 1);
}
