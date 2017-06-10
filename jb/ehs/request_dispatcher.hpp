#ifndef jb_ehs_request_dispatcher_hpp
#define jb_ehs_request_dispatcher_hpp

#include <jb/ehs/base_types.hpp>

#include <atomic>
#include <functional>
#include <map>
#include <mutex>

namespace jb {
namespace ehs {

/**
 * Define the interface for request handlers.
 *
 * An Embedded HTTP Server receives HTTP requests, locates the correct
 * handler for it based on the path component of the URL, and then
 * passes on the request to the handler, which must implement the
 * Callable interface defined by this type.
 */
using request_handler =
    std::function<void(request_type const&, response_type&)>;

/**
 * Holds a collection of HTTP request handlers and forwards requests.
 */
class request_dispatcher {
public:
  /**
   * Create a new empty dispatcher.
   *
   * @param server_name the content of the 'server' HTTP response header.
   */
  explicit request_dispatcher(std::string const& server_name);

  /**
   * Add a new handler.
   *
   * @param path the path that this handler is responsible for.
   * @param handler the handler called when a request hits a path.
   *
   * @throw std::runtime_error if the path already exists
   */
  void add_handler(std::string const& path, request_handler&& handler);

  /**
   * Process a new request using the right handler.
   *
   * Returns the response to send back to the client.  The handler
   * typically creates a normal 200 response, but other responses can
   * be created by the client.  The dispatcher automatically creates
   * 404 responses if the request path does not have a handler
   * registered, and 500 responses if there is an exception handling
   * the request.
   *
   * @param request the HTTP request to process
   * @returns a HTTP response to send back to the client.
   */
  response_type process(request_type const& request) const;

  //@{
  /**
   * Event counters.
   *
   * A series of functions to count interesting events.
   * TODO(coryan) this should really be a separate class that can
   * count any number of metrics, including maps, histograms, rates,
   * etc.  Look at prometheus.io for inspiration.
   */
  /// Count a new connection opened
  void count_open_connection() {
    ++open_connection_;
  }
  /// Returns the count of open connections
  long get_open_connection() const {
    return open_connection_;
  }
  /// Count a new connection closed
  void count_close_connection() {
    ++close_connection_;
  }
  /// Get the count of close connections
  long get_close_connection() const {
    return close_connection_;
  }

  /// Count a successful read
  void count_read_ok() {
    ++read_ok_;
  }
  /// Get the count of successful reads
  long get_read_ok() const {
    return read_ok_;
  }
  /// Count a write errors
  void count_read_error() {
    ++read_error_;
  }
  /// Get the count write errors
  long get_read_error() const {
    return read_error_;
  }

  /// Count a write in the 200 range
  void count_write_200() {
    ++write_200_;
  }
  /// Get the count write in the 200 range
  long get_write_200() const {
    return write_200_;
  }
  /// Count a write in the 300 range
  void count_write_300() {
    ++write_300_;
  }
  /// Get the count write in the 300 range
  long get_write_300() const {
    return write_300_;
  }
  /// Count a write in the 400 range
  void count_write_400() {
    ++write_400_;
  }
  /// Get the count write in the 400 range
  long get_write_400() const {
    return write_400_;
  }
  /// Count a write in the 500 range
  void count_write_500() {
    ++write_500_;
  }
  /// Get the count write in the 500 range
  long get_write_500() const {
    return write_500_;
  }
  /// Count a write errors
  void count_write_ok() {
    ++write_ok_;
  }
  /// Get the count write errors
  long get_write_ok() const {
    return write_ok_;
  }
  /// Count a write errors
  void count_write_error() {
    ++write_error_;
  }
  /// Get the count write errors
  long get_write_error() const {
    return write_error_;
  }
  //@}

private:
  /**
   * Create a 500 response.
   *
   * @param request the request that triggered this response
   * @returns a 500 HTTP response formatted to match the version of
   * the @a request.
   */
  response_type internal_error(request_type const& request) const;

  /**
   * Create a 404 response.
   *
   * @param request the request that triggered this response
   * @returns a 404 HTTP response formatted to match the version of
   * the @a request.
   */
  response_type not_found(request_type const& request) const;

  /**
   * Find the request handler for the given path.
   *
   * @param path the request path
   * @returns a pair with the request handler (if any) and a boolean.
   * The boolean is set to false if the path was not found.
   */
  std::pair<request_handler, bool> find_handler(std::string const& path) const;

private:
  /// Protect the critical sections
  mutable std::mutex mu_;

  /// The collection of handlers
  std::map<std::string, request_handler> handlers_;

  /// The name of the server returned in all HTTP responses.
  std::string server_name_;

  /// Multiple counters
  std::atomic<long> open_connection_;
  std::atomic<long> close_connection_;
  std::atomic<long> read_ok_;
  std::atomic<long> read_error_;
  std::atomic<long> write_200_;
  std::atomic<long> write_300_;
  std::atomic<long> write_400_;
  std::atomic<long> write_500_;
  std::atomic<long> write_ok_;
  std::atomic<long> write_error_;
};

} // namespace ehs
} // namespace jb

#endif // jb_ehs_request_dispatcher_hpp
