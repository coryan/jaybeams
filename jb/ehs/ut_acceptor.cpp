#include <jb/ehs/acceptor.hpp>

#include <thread>

#include <boost/asio/io_service.hpp>
#include <boost/test/unit_test.hpp>

/**
 * @test Verify that jb::ehs::connection + jb::ehs::acceptor work as expected.
 */
BOOST_AUTO_TEST_CASE(acceptor) {
  // Let the operating system pick a listening address ...
  boost::asio::ip::tcp::endpoint ep{boost::asio::ip::address_v4(), 0};

  // ... create a dispatcher and a handler for / ...
  auto dispatcher = std::make_shared<jb::ehs::request_dispatcher>("test");
  using jb::ehs::request_type;
  using jb::ehs::response_type;
  dispatcher->add_handler("/", [](request_type const&, response_type& res) {
    res.fields.insert("Content-type", "text/plain");
    res.body = "OK\n";
  });

  // ... create a IO service, and an acceptor in the default address ...
  boost::asio::io_service io_service;
  jb::ehs::acceptor acceptor(io_service, ep, dispatcher);

  // ... run a separate thread with the io_service so we can write
  // synchronous code in the test, which is easier to follow ...
  std::thread t([&io_service]() { io_service.run(); });

  // ... get the local listening endpoint ...
  auto listen = acceptor.local_endpoint();

  // ... create a separate io service for the client side ...
  boost::asio::io_service io;
  // ... open a socket to send the HTTP request ...
  boost::asio::ip::tcp::socket sock{io};
  sock.connect(listen);
  // ... prepare and send the request ...
  request_type req;
  req.method = "GET";
  req.url = "/";
  req.version = 11;
  req.fields.replace("host", "0.0.0.0");
  req.fields.replace("user-agent", "acceptor_base");
  beast::http::prepare(req);
  beast::http::write(sock, req);
  // ... receive the replay ...
  beast::streambuf sb;
  response_type res;
  beast::http::read(sock, sb, res);

  BOOST_CHECK_EQUAL(res.status, 200);
  BOOST_CHECK_EQUAL(res.version, 11);
  BOOST_CHECK_EQUAL(res.fields["server"], "test");
  BOOST_CHECK_EQUAL(res.body, "OK\n");

  io_service.dispatch([&acceptor, &io_service] {
    acceptor.shutdown();
    io_service.stop();
  });
  t.join();
}
