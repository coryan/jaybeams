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
  acceptor_.close();
  JB_LOG(info) << "shutdown: acceptor close successful";
}

void acceptor::on_accept(boost::system::error_code const& ec) {
  if (not acceptor_.is_open()) {
    // The accept socket is closed, this is a normal condition, used
    // to shutdown the application, so we simply return and do not
    // schedule any additional work ...
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
    // ... this is a very rare condition, the acceptor is still open,
    // but the accept() call failed, typically that would indicate a
    // temporary error, such as running out of file descriptors.  We
    // log the issue, increment the counters, and reschedule another
    // async accept.  Unit testing this condition reliably is
    // extremely hard, so the code is unreached in tests ...
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
