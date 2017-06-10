#include <jb/ehs/acceptor.hpp>

#include <thread>

#include <boost/asio/io_service.hpp>
#include <boost/test/unit_test.hpp>

/**
 * @test Verify that jb::ehs::connection + jb::ehs::acceptor work as expected.
 */
BOOST_AUTO_TEST_CASE(acceptor_base) {
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
  using jb::ehs::request_type;
  using jb::ehs::response_type;
  request_type req;
  req.method = "GET";
  req.url = "/";
  req.version = 11;
  req.fields.replace("host", "0.0.0.0");
  req.fields.replace("user-agent", "acceptor_base");
  beast::http::prepare(req);
  beast::http::write(sock, req);

  // ... receive the reply ...
  beast::streambuf sb;
  response_type res;
  beast::http::read(sock, sb, res);

  BOOST_CHECK_EQUAL(res.status, 200);
  BOOST_CHECK_EQUAL(res.version, 11);
  BOOST_CHECK_EQUAL(res.fields["server"], "test");
  BOOST_CHECK_EQUAL(res.body, "OK\n");

  // ... closing the socket triggers more behaviors in the acceptor
  // and connector classes ...
  sock.close();

  // ... shutdown the acceptor and stop the io_service ...
  io_service.dispatch([&acceptor, &io_service] {
    acceptor.shutdown();
    io_service.stop();
  });
  // ... wait for the acceptor thread ...
  t.join();
}

/**
 * @test Verify that jb::ehs::connection handles read errors.
 */
BOOST_AUTO_TEST_CASE(connection_read_error) {
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
  using jb::ehs::request_type;
  using jb::ehs::response_type;
  request_type req;
  req.method = "GET";
  req.url = "/";
  req.version = 11;
  req.fields.replace("host", "0.0.0.0");
  req.fields.replace("user-agent", "acceptor_base");
  beast::http::prepare(req);
  // ... we lie about the content length to make the server wait for
  // more data ...
  req.fields.replace("content-length", "1000000");
  beast::http::write(sock, req);

  // ... fetch the number of closed connections ...
  long c = dispatcher->get_close_connection();

  // ... and now close the socket before the long message is sent ...
  sock.close();

  // ... wait a few milliseconds for the server to detect the closed
  // connection ...
  while (c == dispatcher->get_close_connection()) {
    std::this_thread::sleep_for(std::chrono::milliseconds(10));
  }
  BOOST_CHECK_EQUAL(c, 0);
  BOOST_CHECK_EQUAL(dispatcher->get_close_connection(), 1);

  // ... shutdown the acceptor and stop the io_service ...
  io_service.dispatch([&acceptor, &io_service] {
    acceptor.shutdown();
    io_service.stop();
  });
  // ... wait for the acceptor thread ...
  t.join();
}

/**
 * @test Verify that jb::ehs::acceptor accepts multiple connections.
 */
BOOST_AUTO_TEST_CASE(acceptor_multiple_connections) {
  // Let the operating system pick a listening address ...
  boost::asio::ip::tcp::endpoint ep{boost::asio::ip::address_v4(), 0};

  // ... create a dispatcher and a handler for / ...
  auto dispatcher = std::make_shared<jb::ehs::request_dispatcher>("test");
  // ... create a IO service, and an acceptor in the default address ...
  boost::asio::io_service io_service;
  jb::ehs::acceptor acceptor(io_service, ep, dispatcher);

  // ... get the local listening endpoint ...
  auto listen = acceptor.local_endpoint();

  // ... create a separate io service for the client side ...
  boost::asio::io_service io;
  // ... open a socket to send the HTTP request ...
  boost::asio::ip::tcp::socket sock{io};
  sock.connect(listen);
  // ... wait a few milliseconds for the server to detect the closed
  // connection ...
  std::this_thread::sleep_for(std::chrono::milliseconds(10));
  sock.close();
  // ... wait a few milliseconds for the server to detect the closed
  // connection ...
  std::this_thread::sleep_for(std::chrono::milliseconds(10));

  // ... wait a few milliseconds for the server to detect the closed
  // connection ...
  std::this_thread::sleep_for(std::chrono::milliseconds(10));
  sock.connect(listen);
  // ... wait a few milliseconds for the server to detect the closed
  // connection ...
  std::this_thread::sleep_for(std::chrono::milliseconds(10));
  sock.close();
  // ... wait a few milliseconds for the server to detect the closed
  // connection ...
  std::this_thread::sleep_for(std::chrono::milliseconds(10));
}

/**
 * @test Verify that jb::ehs::connection can handle multiple requests.
 */
BOOST_AUTO_TEST_CASE(connection_multiple_requests) {
  // Let the operating system pick a listening address ...
  boost::asio::ip::tcp::endpoint ep{boost::asio::ip::address_v4(), 0};

  // ... create a dispatcher and a handler for / ...
  auto dispatcher = std::make_shared<jb::ehs::request_dispatcher>("test");
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
  using jb::ehs::request_type;
  using jb::ehs::response_type;
  request_type req;
  req.method = "GET";
  req.url = "/";
  req.version = 11;
  req.fields.replace("host", "0.0.0.0");
  req.fields.replace("user-agent", "acceptor_base");
  beast::http::prepare(req);
  beast::http::write(sock, req);

  // ... receive the reply ...
  beast::streambuf sb;
  response_type res;
  beast::http::read(sock, sb, res);

  BOOST_CHECK_EQUAL(res.status, 404);
  BOOST_CHECK_EQUAL(res.version, 11);
  BOOST_CHECK_EQUAL(res.fields["server"], "test");

  // ... do the request+reply thing again ...
  beast::http::prepare(req);
  beast::http::write(sock, req);
  beast::http::read(sock, sb, res);
  BOOST_CHECK_EQUAL(res.status, 404);
  BOOST_CHECK_EQUAL(res.version, 11);
  BOOST_CHECK_EQUAL(res.fields["server"], "test");

  // ... shutdown the acceptor and stop the io_service ...
  io_service.dispatch([&acceptor, &io_service] {
    acceptor.shutdown();
    io_service.stop();
  });
  // ... wait for the acceptor thread ...
  t.join();

  // ... closing the socket triggers more behaviors in the acceptor
  // and connector classes ...
  sock.close();
}

/**
 * @test Verify that jb::ehs::acceptor errors paths work.
 */
BOOST_AUTO_TEST_CASE(acceptor_shutdown_errors) {
  // Let the operating system pick a listening address ...
  boost::asio::ip::tcp::endpoint ep{boost::asio::ip::address_v4(), 0};

  // ... create a dispatcher and a handler for / ...
  auto dispatcher = std::make_shared<jb::ehs::request_dispatcher>("test");
  // ... create a IO service, and an acceptor in the default address ...
  boost::asio::io_service io_service;
  jb::ehs::acceptor acceptor(io_service, ep, dispatcher);

  // ... close the acceptor twice, to trigger some of the error code
  // paths ...
  acceptor.shutdown();
  acceptor.shutdown();
}
