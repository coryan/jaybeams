#ifndef jb_ehs_connection_hpp
#define jb_ehs_connection_hpp

#include <jb/ehs/request_dispatcher.hpp>

#include <boost/asio/ip/tcp.hpp>
#include <boost/asio/strand.hpp>
#include <atomic>

namespace jb {
namespace ehs {

/**
 * Handle one connection to the control server.
 */
class connection : public std::enable_shared_from_this<connection> {
public:
  /// The type of socket used for the connection
  using socket_type = boost::asio::ip::tcp::socket;

  /// Constructor
  explicit connection(
      socket_type&& sock, std::shared_ptr<request_dispatcher> dispatcher);

  /// Destructor
  ~connection();

  /// Asynchronously read a HTTP request for this connection.
  void run();

  //@{
  /**
   * Control copy and assignment.
   */
  connection(connection&&) = default;
  connection(connection const&) = default;
  connection& operator=(connection&&) = delete;
  connection& operator=(connection const&) = delete;
  //@}

private:
  /**
   * Handle a completed HTTP request read.
   *
   * Once a HTTP request has been received it needs to be parsed and a
   * response sent back.  This function is called by the Beast
   * framework when the read completes.
   *
   * @param ec the error code
   */
  void on_read(boost::system::error_code const& ec);

  /**
   * Handle a completed response write.
   *
   * @param ec indicate if writing the response resulted in an error.
   */
  void on_write(boost::system::error_code const& ec);

private:
  socket_type sock_;
  std::shared_ptr<request_dispatcher> dispatcher_;
  boost::asio::io_service::strand strand_;
  int id_;
  beast::streambuf sb_;
  request_type req_;

  static std::atomic<int> idgen;
};

} // namespace ehs
} // namespace jb

#endif // jb_ehs_connection_hpp
