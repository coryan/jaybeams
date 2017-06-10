#include "jb/ehs/connection.hpp"

#include <jb/log.hpp>

namespace jb {
namespace ehs {

connection::connection(
    socket_type&& sock, std::shared_ptr<request_dispatcher> dispatcher)
    : sock_(std::move(sock))
    , dispatcher_(dispatcher)
    , strand_(sock_.get_io_service())
    , id_(++idgen) {
  JB_LOG(info) << "#" << id_ << " created, peer=" << sock_.remote_endpoint()
               << ", handle=" << sock_.native_handle();
  dispatcher_->count_open_connection();
}

connection::~connection() {
  JB_LOG(info) << "#" << id_ << " ~connection()";
  dispatcher_->count_close_connection();
}

void connection::run() {
  beast::http::async_read(
      sock_, sb_, req_,
      strand_.wrap(
          [self = shared_from_this()](boost::system::error_code const& ec) {
            self->on_read(ec);
          }));
}

void connection::on_read(boost::system::error_code const& ec) {
  if (ec) {
    JB_LOG(info) << "#" << id_ << " on_read: " << ec.message();
    dispatcher_->count_read_error();
    return;
  }
  dispatcher_->count_read_ok();
  // Prepare a response ...
  response_type res = dispatcher_->process(req_);
  beast::http::prepare(res);
  if (0 <= res.status and res.status < 300) {
    dispatcher_->count_write_200();
  } else if (res.status < 400) {
    dispatcher_->count_write_300();
  } else if (res.status < 500) {
    dispatcher_->count_write_400();
  } else {
    dispatcher_->count_write_500();
  }
  // ... and send it back ...
  beast::http::async_write(
      sock_, std::move(res),
      strand_.wrap(
          [self = shared_from_this()](boost::system::error_code const& ec) {
            self->on_write(ec);
          }));
}

void connection::on_write(boost::system::error_code const& ec) {
  if (ec) {
    JB_LOG(info) << "#" << id_ << " on_write: " << ec.message();
    dispatcher_->count_write_error();
    return;
  }
  // Start a new asynchronous read for the next message ...
  run();
}

// Define the generator for connection ids ...
std::atomic<int> connection::idgen(0);

} // namespace ehs
} // namespace jb
