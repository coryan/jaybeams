#include <jb/ehs/request_dispatcher.hpp>

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
  tested.add_handler("/error", [](request_type const& req, response_type& res) {
    throw std::runtime_error("bad stuff happens");
  });

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
}
