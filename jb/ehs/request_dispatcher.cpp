#include "jb/ehs/request_dispatcher.hpp"
#include <jb/log.hpp>

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
    , write_error_(0) {
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

response_type request_dispatcher::process(request_type const& req) {
  try {
    auto found = find_handler(req.url);
    if (not found.second) {
      return not_found(req);
    }
    response_type res;
    res.status = 200;
    res.reason = beast::http::reason_string(res.status);
    res.version = req.version;
    res.fields.insert("server", server_name_);
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
}

response_type request_dispatcher::internal_error(request_type const& req) {
  response_type res;
  res.status = 500;
  res.reason = beast::http::reason_string(res.status);
  res.version = req.version;
  res.fields.insert("server", server_name_);
  res.fields.insert("content-type", "text/plain");
  res.body = std::string{"An internal error occurred"};
  update_response_counter(res);
  return res;
}

response_type request_dispatcher::not_found(request_type const& req) {
  response_type res;
  res.status = 404;
  res.reason = beast::http::reason_string(res.status);
  res.version = req.version;
  res.fields.insert("server", server_name_);
  res.fields.insert("content-type", "text/plain");
  res.body = std::string("path: " + req.url + " not found\r\n");
  update_response_counter(res);
  return res;
}

std::pair<request_handler, bool>
request_dispatcher::find_handler(std::string const& path) const {
  std::lock_guard<std::mutex> guard(mu_);
  auto location = handlers_.find(path);
  if (location == handlers_.end()) {
    return std::make_pair(request_handler(), false);
  }
  return std::make_pair(location->second, true);
}

void request_dispatcher::update_response_counter(response_type const& res) {
  // TODO(coryan) - this sound become a map of counter (or is that a
  // counter map?) when we create counter classes for prometheus.io
  if (res.status < 100) {
    count_write_invalid();
  } else if (res.status < 200) {
    count_write_100();
  } else if (res.status < 300) {
    count_write_200();
  } else if (res.status < 400) {
    count_write_300();
  } else if (res.status < 500) {
    count_write_400();
  } else if (res.status < 600) {
    count_write_500();
  } else {
    count_write_invalid();
  }
}

} // namespace ehs
} // namespace jb
