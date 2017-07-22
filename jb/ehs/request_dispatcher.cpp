#include "jb/ehs/request_dispatcher.hpp"
#include <jb/log.hpp>

#include <sstream>

namespace jb {
namespace ehs {

request_dispatcher::request_dispatcher(std::string const& server_name)
    : mu_()
    , handlers_()
    , server_name_(server_name)
    , open_connection_(0)
    , close_connection_(0)
    , read_ok_(0)
    , read_error_(0)
    , write_invalid_(0)
    , write_100_(0)
    , write_200_(0)
    , write_300_(0)
    , write_400_(0)
    , write_500_(0)
    , write_ok_(0)
    , write_error_(0)
    , accept_ok_(0)
    , accept_error_(0)
    , accept_closed_(0) {
#ifndef ATOMIC_LONG_LOCK_FREE
#error "Missing ATOMIC_LONG_LOCK_FREE required by C++11 standard"
#endif // ATOMIC_LONG_LOCK_FREE
  static_assert(
      ATOMIC_LONG_LOCK_FREE == 2, "Class requires lock-free std::atomic<long>");
}

void request_dispatcher::add_handler(
    std::string const& path, request_handler&& handler) {
  std::lock_guard<std::mutex> guard(mu_);
  auto inserted = handlers_.emplace(path, std::move(handler));
  if (not inserted.second) {
    throw std::runtime_error(std::string("duplicate handler path: ") + path);
  }
}

response_type request_dispatcher::process(request_type const& req) try {
  auto found = find_handler(req.target());
  if (not found.second) {
    return not_found(req);
  }
  response_type res;
  res.result(beast::http::status::ok);
  res.version = req.version;
  res.insert("server", server_name_);
  found.first(req, res);
  update_response_counter(res);
  return res;
} catch (std::exception const& e) {
  // ... if there is an exception preparing the response we try to
  // send back at least a 500 error ...
  JB_LOG(info) << "std::exception raised while sending response: "
               << e.what();
  return internal_error(req);
}

void request_dispatcher::append_metrics(response_type& res) const {
  std::ostringstream os;
  os << "# HELP open_connection The number of HTTP connections opened\n"
     << "# TYPE open_connection counter\n"
     << "open_connection " << get_open_connection() << "\n"
     << "\n"
     << "# HELP close_connection The number of HTTP connections closed\n"
     << "# TYPE close_connection counter\n"
     << "close_connection " << get_close_connection() << "\n"
     << "\n"
     << "# HELP read_ok The number of HTTP request received successfully\n"
     << "# TYPE read_ok counter\n"
     << "read_ok " << get_read_ok() << "\n"
     << "\n"
     << "# HELP read_error The number of errors reading HTTP requests\n"
     << "# TYPE read_error counter\n"
     << "read_error " << get_read_error() << "\n"
     << "\n"
     << "# HELP response_by_code_range "
     << "The number of HTTP responses within each response code range\n"
     << "# TYPE response_by_code_range\n"
     << "response_by_code_range{range=\"invalid\"} " << get_write_invalid()
     << "\n"
     << "response_by_code_range{range=\"100\"} " << get_write_100() << "\n"
     << "response_by_code_range{range=\"200\"} " << get_write_200() << "\n"
     << "response_by_code_range{range=\"300\"} " << get_write_300() << "\n"
     << "response_by_code_range{range=\"400\"} " << get_write_400() << "\n"
     << "response_by_code_range{range=\"500\"} " << get_write_500() << "\n"
     << "\n"
     << "# HELP write_ok The number of HTTP responses received successfully\n"
     << "# TYPE write_ok counter\n"
     << "write_ok " << get_write_ok() << "\n\n"
     << "# HELP write_error The number of errors writing HTTP responses\n"
     << "# TYPE write_error counter\n"
     << "write_error " << get_write_error() << "\n"
     << "\n"
     << "# HELP accept_ok The number of HTTP connections accepted\n"
     << "# TYPE accept_ok counter\n"
     << "accept_ok " << get_accept_ok() << "\n\n"
     << "# HELP accept_error The number of errors accepting HTTP connections\n"
     << "# TYPE accept_error counter\n"
     << "accept_error " << get_accept_error() << "\n"
     << "# HELP accept_closed The number accept() attempts on a closed "
        "acceptor\n"
     << "# TYPE accept_closed counter\n"
     << "accept_closed " << get_accept_closed() << "\n"
     << "\n";
  res.body += os.str();
}

response_type request_dispatcher::internal_error(request_type const& req) {
  response_type res;
  res.result(beast::http::status::internal_server_error);
  res.version = req.version;
  res.insert("server", server_name_);
  res.insert("content-type", "text/plain");
  res.body = std::string{"An internal error occurred"};
  update_response_counter(res);
  return res;
}

response_type request_dispatcher::not_found(request_type const& req) {
  response_type res;
  res.result(beast::http::status::not_found);
  res.version = req.version;
  res.insert("server", server_name_);
  res.insert("content-type", "text/plain");
  res.body =
      std::string("path: ") + std::string(req.target()) + " not found\r\n";
  update_response_counter(res);
  return res;
}

std::pair<request_handler, bool>
request_dispatcher::find_handler(beast::string_view path) const {
  std::lock_guard<std::mutex> guard(mu_);
  auto location = handlers_.find(std::string(path));
  if (location == handlers_.end()) {
    return std::make_pair(request_handler(), false);
  }
  return std::make_pair(location->second, true);
}

void request_dispatcher::update_response_counter(response_type const& res) {
  // TODO(coryan) - this sound become a map of counter (or is that a
  // counter map?) when we create counter classes for prometheus.io
  using sc = beast::http::status_class;
  switch (beast::http::to_status_class(res.result())) {
  case sc::unknown:
    count_write_invalid();
    break;
  case sc::informational:
    count_write_100();
    break;
  case sc::successful:
    count_write_200();
    break;
  case sc::redirection:
    count_write_300();
    break;
  case sc::client_error:
    count_write_400();
    break;
  case sc::server_error:
    count_write_500();
    break;
  }
}

} // namespace ehs
} // namespace jb
