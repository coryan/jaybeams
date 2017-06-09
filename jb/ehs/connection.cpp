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
    return;
  }
  // Prepare a response ...
  response_type res = dispatcher_->process(req_);
  beast::http::prepare(res);
  // ... and send it back ...
  beast::http::async_write(
      sock_, std::move(res),
      [self = shared_from_this()](boost::system::error_code const& ec) {
        self->on_write(ec);
      });
}

void connection::on_write(boost::system::error_code const& ec) {
  if (ec) {
    JB_LOG(info) << "#" << id_ << " on_write: " << ec.message();
    return;
  }
  // Start a new asynchronous read for the next message ...
  run();
}

// Define the generator for connection ids ...
std::atomic<int> connection::idgen(0);

} // namespace ehs
} // namespace jb
