#ifndef jb_ehs_acceptor_hpp
#define jb_ehs_acceptor_hpp

#include <jb/ehs/connection.hpp>

#include <boost/asio/ip/tcp.hpp>

namespace jb {
namespace ehs {

/**
 * Create a control server for the program.
 *
 * The program runs as a typical daemon, accepting HTTP requests to
 * start new replays, stop them, and show its current status.
 */
class acceptor {
public:
  /**
   * Create an acceptor a Embedded HTTP Server and start accepting connections.
   *
   * @param io Boost.ASIO service used to demux I/O events for this
   * acceptor.
   * @param ep the endpoint this control server listens on.
   * @param dispatcher the object to process requests.
   */
  acceptor(
      boost::asio::io_service& io, boost::asio::ip::tcp::endpoint const& ep,
      std::shared_ptr<request_dispatcher> dispatcher);

  /// Return the local listening endpoint.
  boost::asio::ip::tcp::endpoint local_endpoint() const {
    return acceptor_.local_endpoint();
  }

  /// Gracefully shutdown the acceptor
  void shutdown();

private:
  /**
   * Handle a completed asynchronous accept() call.
   */
  void on_accept(boost::system::error_code const& ec);

private:
  boost::asio::ip::tcp::acceptor acceptor_;
  std::shared_ptr<request_dispatcher> dispatcher_;
  // Hold the results of an  asynchronous accept() operation.
  boost::asio::ip::tcp::socket sock_;
};

} // namespace ehs
} // namespace jb

#endif // jb_ehs_acceptor_hpp
