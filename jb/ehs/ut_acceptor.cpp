#include <jb/ehs/acceptor.hpp>

#include <thread>

#include <boost/asio/io_service.hpp>
#include <boost/test/unit_test.hpp>

/// Wait for a connection close event in the dispatcher
namespace {
void wait_for_connection_close(
    std::shared_ptr<jb::ehs::request_dispatcher> d, long last_count) {
  // .. wait until the connection is closed ...
  for (int c = 0; c != 10 and last_count == d->get_close_connection(); ++c) {
    std::this_thread::sleep_for(std::chrono::milliseconds(10));
  }
  BOOST_CHECK_EQUAL(d->get_close_connection(), last_count + 1);
}
} // anonymous namespace

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
  wait_for_connection_close(dispatcher, dispatcher->get_close_connection());

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
  for (int i = 0; i != 10 and c == dispatcher->get_close_connection(); ++i) {
    std::this_thread::sleep_for(std::chrono::milliseconds(10));
  }
  BOOST_CHECK_EQUAL(c, 0);
  BOOST_CHECK_EQUAL(dispatcher->get_close_connection(), 1);
  BOOST_CHECK_EQUAL(dispatcher->get_read_error(), 1);

  // ... shutdown the acceptor and stop the io_service ...
  io_service.dispatch([&acceptor, &io_service] {
    acceptor.shutdown();
    io_service.stop();
  });
  // ... wait for the acceptor thread ...
  t.join();
}

namespace {
/// Open and close a connection to @a ep
void cycle_connection(
    std::shared_ptr<jb::ehs::request_dispatcher> d, boost::asio::io_service& io,
    boost::asio::ip::tcp::endpoint const& ep, long expected_open_count,
    long expected_close_count) {

  // ... get the number of open connections so far ...
  auto open_count = d->get_open_connection();
  BOOST_CHECK_EQUAL(open_count, expected_open_count);

  // ... open a socket to send the HTTP request ...
  boost::asio::ip::tcp::socket sock{io};
  sock.connect(ep);

  // ... wait until the connection is received ...
  for (int c = 0; c != 10 and open_count == d->get_open_connection(); ++c) {
    std::this_thread::sleep_for(std::chrono::milliseconds(10));
  }
  BOOST_CHECK_EQUAL(d->get_open_connection(), expected_open_count + 1);

  // ... get the number of closed connections so far ...
  auto close_count = d->get_close_connection();
  BOOST_CHECK_EQUAL(close_count, expected_close_count);

  // ... close the socket ...
  sock.close();
  wait_for_connection_close(d, close_count);
}

} // anonymous namespace

/**
 * @test Verify that jb::ehs::acceptor accepts multiple connections.
 */
BOOST_AUTO_TEST_CASE(acceptor_multiple_connections) {
  // Let the operating system pick a listening address ...
  boost::asio::ip::tcp::endpoint ep{boost::asio::ip::address_v4(), 0};

  // ... create a dispatcher, with no handlers ...
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
  cycle_connection(dispatcher, io, listen, 0, 0);
  cycle_connection(dispatcher, io, listen, 1, 1);

  // ... shutdown the acceptor and stop the io_service ...
  io_service.dispatch([&acceptor, &io_service] {
    acceptor.shutdown();
    io_service.stop();
  });
  // ... wait for the acceptor thread ...
  t.join();
}

/**
 * @test Verify that jb::ehs::connection can handle multiple requests.
 */
BOOST_AUTO_TEST_CASE(connection_multiple_requests) {
  // Let the operating system pick a listening address ...
  boost::asio::ip::tcp::endpoint ep{boost::asio::ip::address_v4(), 0};

  // ... create a dispatcher, with no handlers ...
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

  // ... closing the socket triggers more behaviors in the acceptor
  // and connector classes ...
  auto close_count = dispatcher->get_close_connection();
  sock.close();
  wait_for_connection_close(dispatcher, close_count);

  // ... shutdown the acceptor and stop the io_service ...
  io_service.dispatch([&acceptor, &io_service] {
    acceptor.shutdown();
    io_service.stop();
  });
  // ... wait for the acceptor thread ...
  t.join();
}

/**
 * @test Verify that jb::ehs::connection handles errors during writes.
 */
BOOST_AUTO_TEST_CASE(connection_write_errors) {
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
  // ... create a IO service, and an acceptor in the default address,
  // and two threads to process events in them ...
  boost::asio::io_service io_service;
  jb::ehs::acceptor acceptor(io_service, ep, dispatcher);
  std::thread t0([&io_service]() { io_service.run(); });

  // ... do all the stuff to connect to the service ...
  auto listen = acceptor.local_endpoint();
  boost::asio::io_service io;
  boost::asio::ip::tcp::socket sock{io};

  // ... this is a weird handler.  It closes the incoming socket as
  // soon as it receives a request.  That should force an error during
  // the write path ...
  dispatcher->add_handler(
      "/close-me",
      [&sock, &io_service](request_type const&, response_type& res) {
        // ... close the socket, that is evil ...
        sock.close();
        // ... create a largish message ...
        res.fields.insert("Content-type", "text/plain");
        std::string block("Good luck with that\n");
        block.append(1000000, ' ');
        res.body = std::move(block);
      });

  // ... try multiple times, sometimes the on_read() callback is
  // called before the on_write() callback ...
  for (int i = 0; i != 10; ++i) {
    BOOST_TEST_CHECKPOINT("Test attempt #" << i);
    sock.connect(listen);
    // ... send a request ...
    request_type req;
    req.method = "GET";
    req.url = "/close-me";
    req.version = 11;
    req.fields.replace("host", "0.0.0.0");
    req.fields.replace("user-agent", "acceptor_base");
    beast::http::prepare(req);
    beast::http::write(sock, req);

    // ... try to receive the reply, but the socket will close while
    // doing so ...
    beast::streambuf sb;
    response_type res;
    boost::system::error_code ec;
    beast::http::read(sock, sb, res, ec);

    BOOST_CHECK(bool(ec));
    BOOST_TEST_MESSAGE(
        "While reading HTTP response got: " << ec.message() << " ["
                                            << ec.category().name() << "/"
                                            << ec.value() << "]");

    // ... wait until the dispatcher closes its connection ...
    for (int c = 0; c != 10 and dispatcher->get_write_error() == 0; ++c) {
      std::this_thread::sleep_for(std::chrono::milliseconds(10));
    }
    if (dispatcher->get_write_error() != 0) {
      break;
    }
  }
  for (int c = 0; c != 100 and dispatcher->get_close_connection() == 0; ++c) {
    std::this_thread::sleep_for(std::chrono::milliseconds(10));
  }
  BOOST_CHECK_GE(dispatcher->get_write_error(), 1);
  BOOST_CHECK_GE(dispatcher->get_close_connection(), 1);

  // ... shutdown everything ...
  io_service.dispatch([&acceptor, &io_service] {
    acceptor.shutdown();
    io_service.stop();
  });
  t0.join();
}

/**
 * @test Verify that jb::ehs::acceptor shutdown is safe to call twice.
 */
BOOST_AUTO_TEST_CASE(acceptor_double_shutdown) {
  // Let the operating system pick a listening address ...
  boost::asio::ip::tcp::endpoint ep{boost::asio::ip::address_v4(), 0};

  // ... create a dispatcher, with no handlers ...
  auto dispatcher = std::make_shared<jb::ehs::request_dispatcher>("test");
  // ... create a IO service, and an acceptor in the default address ...
  boost::asio::io_service io_service;
  jb::ehs::acceptor acceptor(io_service, ep, dispatcher);

  acceptor.shutdown();
  acceptor.shutdown();

  BOOST_CHECK_EQUAL(dispatcher->get_accept_error(), 0);
}
