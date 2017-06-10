#include "jb/ehs/acceptor.hpp"

#include <jb/log.hpp>

namespace jb {
namespace ehs {

acceptor::acceptor(
    boost::asio::io_service& io, boost::asio::ip::tcp::endpoint const& ep,
    std::shared_ptr<request_dispatcher> dispatcher)
    : acceptor_(io)
    , dispatcher_(dispatcher)
    , sock_(io) {
  acceptor_.open(ep.protocol());
  acceptor_.bind(ep);
  acceptor_.listen(boost::asio::socket_base::max_connections);
  // ... schedule an asynchronous accept() operation into the new
  // socket ...
  acceptor_.async_accept(sock_, [this](boost::system::error_code const& ec) {
    this->on_accept(ec);
  });
  JB_LOG(info) << "accepting connections on " << ep << " [" << local_endpoint()
               << "]";
}

void acceptor::shutdown() {
  boost::system::error_code ec;
  acceptor_.close(ec);
  if (ec) {
    JB_LOG(info) << "shutdown: " << ec.message() << " [" << ec.category().name()
                 << "/" << ec.value() << "]";
  } else {
    JB_LOG(info) << "shutdown: acceptor close successful";
  }
}

void acceptor::on_accept(boost::system::error_code const& ec) {
  // Return when the acceptor is closed, there is no more work to do
  // in this case ...
  if (not acceptor_.is_open()) {
    JB_LOG(info) << "on_accept: acceptor is not open";
    return;
  }
  // ... move the newly created socket to a stack variable so we can
  // schedule a new asynchronous accept ...
  boost::asio::ip::tcp::socket sock(std::move(sock_));
  if (not ec) {
    // ... create a new connection for the newly created socket and
    // schedule it ...
    auto c = std::make_shared<connection>(std::move(sock), dispatcher_);
    c->run();
    dispatcher_->count_accept_ok();
  } else {
    JB_LOG(info) << "on_accept: " << ec.message() << " ["
                 << ec.category().name() << "/" << ec.value() << "]";
    dispatcher_->count_accept_error();
  }
  // ... error or not, schedule a new accept() request ..
  acceptor_.async_accept(sock_, [this](boost::system::error_code const& ec) {
    this->on_accept(ec);
  });
}

} // namespace ehs
} // namespace jb
