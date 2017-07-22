#include <jb/ehs/acceptor.hpp>

#include <thread>

#include <boost/asio/io_service.hpp>
#include <boost/test/unit_test.hpp>

/// Wait for a connection close event in the dispatcher
namespace {
void wait_for_connection_close(
    std::shared_ptr<jb::ehs::request_dispatcher> d, long last_count) {
  using namespace std::chrono_literals;
  // .. wait until the connection is closed ...
  for (int c = 0; c != 100 and last_count == d->get_close_connection(); ++c) {
    std::this_thread::sleep_for(10ms);
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
    res.insert("Content-type", "text/plain");
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
  request_type req{beast::http::verb::get, "/", 11};
  req.set(beast::http::field::host, "0.0.0.0");
  req.set(beast::http::field::user_agent, "acceptor_base");
  // req.set(beast::http::field::connection, "close");
  beast::http::write(sock, req);

  // ... receive the reply ...
  beast::flat_buffer sb{8192};
  response_type res;
  beast::http::read(sock, sb, res);

  BOOST_CHECK_EQUAL(res.result_int(), 200);
  BOOST_CHECK_EQUAL(res.version, 11);
  BOOST_CHECK_EQUAL(res["server"], "test");
  BOOST_CHECK_EQUAL(res.body, "OK\n");

  // ... closing the socket triggers more behaviors in the acceptor
  // and connector classes ...
  auto current = dispatcher->get_close_connection();
  sock.close();
  BOOST_TEST_CHECKPOINT("closing connection in acceptor_base");
  wait_for_connection_close(dispatcher, current);

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
  using namespace std::chrono_literals;
  // Let the operating system pick a listening address ...
  boost::asio::ip::tcp::endpoint ep{boost::asio::ip::address_v4(), 0};

  // ... create a dispatcher and a handler for / ...
  auto dispatcher = std::make_shared<jb::ehs::request_dispatcher>("test");
  using jb::ehs::request_type;
  using jb::ehs::response_type;
  dispatcher->add_handler("/", [](request_type const&, response_type& res) {
    res.insert("Content-type", "text/plain");
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
  request_type req{beast::http::verb::get, "/", 11};
  req.set("host", "0.0.0.0");
  req.set("user-agent", "acceptor_base");
  // ... we lie about the content length to make the server wait for
  // more data ...
  req.set(beast::http::field::content_length, "1000000");
  beast::http::write(sock, req);

  // ... fetch the number of closed connections ...
  long c = dispatcher->get_close_connection();

  // ... and now close the socket before the long message is sent ...
  sock.close();

  // ... wait a few milliseconds for the server to detect the closed
  // connection ...
  for (int i = 0; i != 10 and c == dispatcher->get_close_connection(); ++i) {
    std::this_thread::sleep_for(10ms);
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
  using namespace std::chrono_literals;

  // ... get the number of open connections so far ...
  auto open_count = d->get_open_connection();
  BOOST_CHECK_EQUAL(open_count, expected_open_count);

  // ... open a socket to send the HTTP request ...
  boost::asio::ip::tcp::socket sock{io};
  sock.connect(ep);

  // ... wait until the connection is received ...
  for (int c = 0; c != 10 and open_count == d->get_open_connection(); ++c) {
    std::this_thread::sleep_for(10ms);
  }
  BOOST_CHECK_EQUAL(d->get_open_connection(), expected_open_count + 1);

  // ... get the number of closed connections so far ...
  auto close_count = d->get_close_connection();
  BOOST_CHECK_EQUAL(close_count, expected_close_count);

  // ... close the socket ...
  sock.close();
  BOOST_TEST_CHECKPOINT("closing connection in cycle_connection");
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
  request_type req{beast::http::verb::get, "/", 11};
  req.set("host", "0.0.0.0");
  req.set("user-agent", "acceptor_base");
  beast::http::write(sock, req);

  // ... receive the reply ...
  beast::flat_buffer sb;
  response_type res;
  beast::http::read(sock, sb, res);

  BOOST_CHECK_EQUAL(res.result_int(), 404);
  BOOST_CHECK_EQUAL(res.version, 11);
  BOOST_CHECK_EQUAL(res["server"], "test");

  // ... do the request+reply thing again ...
  beast::http::write(sock, req);
  beast::http::read(sock, sb, res);
  BOOST_CHECK_EQUAL(res.result_int(), 404);
  BOOST_CHECK_EQUAL(res.version, 11);
  BOOST_CHECK_EQUAL(res["server"], "test");

  // ... closing the socket triggers more behaviors in the acceptor
  // and connector classes ...
  auto close_count = dispatcher->get_close_connection();
  sock.close();
  BOOST_TEST_CHECKPOINT("closing connection in connection_multiple_requests");
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

/**
 * @test Improve coverage forjb::ehs::acceptor.
 */
BOOST_AUTO_TEST_CASE(acceptor_on_accept_closed) {
  using namespace std::chrono_literals;
  // Let the operating system pick a listening address ...
  boost::asio::ip::tcp::endpoint ep{boost::asio::ip::address_v4(), 0};

  // ... create a dispatcher, with no handlers ...
  auto dispatcher = std::make_shared<jb::ehs::request_dispatcher>("test");
  // ... create a IO service, and an acceptor in the default address ...
  boost::asio::io_service io_service;

  // ... the acceptor will register for the on_accept() callback ...
  jb::ehs::acceptor acceptor(io_service, ep, dispatcher);

  // ... close the acceptor before it has a chance to do anything ...
  acceptor.shutdown();

  // ... the thread will call on_accept() just because it is closed,
  // and that will hit one more path ...
  std::thread t([&io_service]() { io_service.run(); });

  for (int c = 0; c != 100 and 0 == dispatcher->get_accept_closed(); ++c) {
    std::this_thread::sleep_for(10ms);
  }
  BOOST_CHECK_EQUAL(dispatcher->get_accept_closed(), 1);

  // ... shutdown everything ...
  io_service.dispatch([&acceptor, &io_service] { io_service.stop(); });
  t.join();
}
