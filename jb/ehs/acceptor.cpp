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
  JB_LOG(info) << "accepting connections on " << ep;
}

void acceptor::on_accept(boost::system::error_code const& ec) {
  // Return when the acceptor is closed or there is an error ...
  if (not acceptor_.is_open()) {
    return;
  }
  if (ec) {
    JB_LOG(info) << "accept: " << ec.message();
    return;
  }
  // ... move the newly created socket to a stack variable so we can
  // schedule a new asynchronous accept ...
  boost::asio::ip::tcp::socket sock(std::move(sock_));
  acceptor_.async_accept(sock_, [this](boost::system::error_code const& ec) {
    this->on_accept(ec);
  });
  // ... create a new connection for the newly created socket and
  // schedule it ...
  auto c = std::make_shared<connection>(std::move(sock), dispatcher_);
  c->run();
}

} // namespace ehs
} // namespace jb
