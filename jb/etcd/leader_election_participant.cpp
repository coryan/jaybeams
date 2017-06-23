#include "jb/etcd/leader_election_participant.hpp"
#include <jb/assert_throw.hpp>
#include <jb/log.hpp>

#include <google/protobuf/text_format.h>
#include <iomanip>
#include <sstream>
#include <stdexcept>

namespace jb {
namespace etcd {

// Notice that this constructor does not kick-off the campaign, it is
// a private constructor, it captures the shared code between the
// other constructors.  Each one of them kicks off the campaign is a
// slightly different way ...
leader_election_participant::leader_election_participant(
    bool shared, std::shared_ptr<client_factory> client,
    std::string const& etcd_endpoint, std::string const& election_name,
    std::uint64_t lease_id, std::string const& participant_value)
    : client_(client)
    , channel_(client->create_channel(etcd_endpoint))
    , kv_client_(client_->create_kv(channel_))
    , watch_client_(client_->create_watch(channel_))
    , watcher_stream_()
    , queue_(std::make_shared<completion_queue>())
    , election_name_(election_name)
    , lease_id_(lease_id)
    , participant_value_(participant_value)
    , election_prefix_(election_name + "/")
    , participant_key_([this]() {
      // ... build the key using the session's lease.  The lease id is
      // unique, assigned by etcd, so each participant gets a different
      // key.  The use of an inline functor is because why not ...
      // TODO() - document why there can be no accidental conflict with
      // another election running in the system.
      std::ostringstream os;
      os << election_prefix_ << std::hex << this->lease_id_;
      return os.str();
    }())
    , pending_watches_(0)
    , campaign_result_()
    , campaign_callback_()
    , completion_queue_loop_() {
  // ... start the event loop, if an exception is raised we will need
  // to shutdown the event loop and join the thread, otherwise the
  // destructor of the fully initialized thread object
  // (completion_event_loop_ or a temporary) will call std::terminate() ...
  completion_queue_loop_ = std::thread([this]() { run(); });
  // ... this is just the initialization after all the member
  // variables are set.  Moved to a function because the code gets too
  // big otherwise ...
  preamble();
}

leader_election_participant::~leader_election_participant() {
  shutdown();
}

void leader_election_participant::resign() {
}

void leader_election_participant::proclaim(std::string const& new_value) {
  std::string copy(new_value);
  etcdserverpb::RequestOp failure_op;
  auto result = publish_value(copy, failure_op);
  // TODO() - the failure_op should be a Get() and we need to decide
  // what to do when this fails ...
  copy.swap(participant_value_);
}

void leader_election_participant::preamble() try {
  // ... need to create the watcher stream, we do this here, and block
  // until it is done, because it would be a pain to manage the state
  // machine where the stream may or may not be setup and other
  // operations are executing ...
  using W = etcdserverpb::WatchRequest;
  using R = etcdserverpb::WatchResponse;
  using watch_stream = async_rdwr_stream<W, R>;
  static_assert(
      std::is_same<
          decltype(watcher_stream_),
          std::unique_ptr<watch_stream::client_type>>::value,
      "Mismatched async stream for Watcher");
  std::promise<std::unique_ptr<watch_stream::client_type>> watcher_stream_ready;
  auto op = make_async_rdwr_stream<W, R>([&watcher_stream_ready](auto op) {
    watcher_stream_ready.set_value(std::move(op->stream));
  });
  watch_client_->AsyncWatch(&op->context, *queue_, op->tag());
  // ... blocks until it is ready ...
  watcher_stream_ = watcher_stream_ready.get_future().get();

  // ... we need to create a node to represent this participant in
  // the leader election.  We do this with a test-and-set
  // operation.  The test is "does this key have creation_version ==
  // 0", which is really equivalent to "does this key exist",
  // because any key actually created would have a higher creation
  // version ...
  etcdserverpb::TxnRequest req;
  auto& cmp = *req.add_compare();
  cmp.set_key(key());
  cmp.set_result(etcdserverpb::Compare::EQUAL);
  cmp.set_target(etcdserverpb::Compare::CREATE);
  cmp.set_create_revision(0);
  // ... if the key is not there we are going to create it, and
  // store the "participant value" there ...
  auto& on_success = *req.add_success()->mutable_request_put();
  on_success.set_key(key());
  on_success.set_value(value());
  on_success.set_lease(lease_id());
  // ... if the key is there, we are going to fetch its current
  // value, there will be some fun action with that ...
  auto& on_failure = *req.add_failure()->mutable_request_range();
  on_failure.set_key(key());

  // ... execute the transaction in etcd ...
  etcdserverpb::TxnResponse resp = commit(req);

  // ... regardless of which branch of the test-and-set operation
  // pass, we now have fetched the participant revision value ..
  participant_revision_ = resp.header().revision();

  if (not resp.succeeded()) {
    // ... the key already existed, possibly because a previous
    // instance of the program participated in the election and etcd
    // did not had time to expire the key.  We need to use the
    // previous creation_revision and save our new participant_value
    // ...
    JB_ASSERT_THROW(resp.responses().size() == 1);
    JB_ASSERT_THROW(resp.responses()[0].response_range().kvs().size() == 1);
    auto const& kv = resp.responses()[0].response_range().kvs()[0];
    participant_revision_ = kv.create_revision();
    // ... if the value is the same, we can avoid a round-trip
    // request to the server ...
    if (kv.value() != value()) {
      // ... too bad, need to publish again *and* we need to delete
      // the key if the publication fails ...
      etcdserverpb::RequestOp failure_op;
      auto& delete_op = *failure_op.mutable_request_delete_range();
      delete_op.set_key(key());
      auto published = publish_value(value(), failure_op);
      if (not published.succeeded()) {
        // ... ugh, the publication failed.  We now have an
        // inconsistent state with the server.  We think we own the
        // code (and at least we own the lease!), but we were unable
        // to publish the value.  We are going to raise an exception
        // and abort the creation of the participant object ...
        std::ostringstream os;
        os << "Unexpected failure writing new value on existing key=" << key()
           << "\n";
        std::string print;
        google::protobuf::TextFormat::PrintToString(published, &print);
        os << "txn result=" << print << "\n";
        throw std::runtime_error(os.str());
      }
    }
  }
} catch (std::exception const& ex) {
  JB_LOG(info) << "Standard exception raised in preamble: " << ex.what();
  shutdown();
  throw;
} catch (...) {
  JB_LOG(info) << "Unknown exception raised in preamble";
  shutdown();
  throw;
}

void leader_election_participant::shutdown() {
  if (watcher_stream_) {
    // The watcher stream was already created, we need to close it
    // before shutting down the completion queue ...
    std::promise<bool> watcher_stream_closed;
    auto op = make_writes_done_op([this, &watcher_stream_closed](auto op) {
      on_writes_done(op, watcher_stream_closed);
    });
    watcher_stream_->WritesDone(op->tag());
    // ... block until it closes ...
    watcher_stream_closed.get_future().get();
    watcher_stream_.reset();
  }
  queue_->shutdown();
  completion_queue_loop_.join();
}

void leader_election_participant::campaign() {
  // We are going to wait using a local promise ...
  std::promise<bool> elected;
  // ... this will kick-off the campaign, a separate thread runs the
  // asynchronous operations to wait until the campaign is done.  It
  // will call the provided lambda when it is done ...
  campaign([&elected](std::future<bool>& result) {
    // ... the callback receives a future, hopefully with "true"
    // indicating the election was successful, but it could be an
    // exception.  We capture both case ..
    try {
      elected.set_value(result.get());
    } catch (...) {
      elected.set_exception(std::current_exception());
    }
  });
  // ... block until the promise is satisfied, notice that get()
  // raises the exception if that was the result ...
  if (elected.get_future().get() != true) {
    // ... we also raise if the campaign failed ...
    std::ostringstream os;
    os << "Unexpected false value after running campaign. "
       << " participant_key=" << participant_key_;
    throw std::runtime_error(os.str());
  }
}

void leader_election_participant::campaign_impl(
    std::function<void(std::future<bool>&)>&& callback) {
  // First save the callback ...
  {
    std::lock_guard<std::mutex> lock(mu_);
    JB_ASSERT_THROW(bool(campaign_callback_) == false);
    campaign_callback_ = std::move(callback);
  }
  // ... we want to wait on a single key, waiting on more would
  // create thundering herd problems.  To win the election this
  // participant needs to have the smallest creation_revision
  // amongst all the participants within the election.

  // So we wait on the immediate predecessor of the current
  // participant sorted by creation_revision.  That is found by:
  etcdserverpb::RangeRequest req;
  //   - Search all the keys that have the same prefix.
  //   - Prefix searches are range searches where the end value is 1
  //     bit higher than the key
  req.set_key(key());
  req.set_range_end(prefix_end(key()));
  //   - Limit those results to the keys that have creation_revision
  //     lower than this participants creation_revision key
  req.set_max_create_revision(participant_revision_ - 1);
  //   - Sort those results in descending order by
  //     creation_revision.
  req.set_sort_order(etcdserverpb::RangeRequest::DESCEND);
  req.set_sort_target(etcdserverpb::RangeRequest::CREATE);
  //   - Only fetch the first of those results.
  req.set_limit(1);

  // ... after all that filtering you are left with 0 or 1 keys.
  // If there is 1 key, we need to setup a watcher and wait until
  // the key is deleted.
  // If there are 0 keys, we won the campaign, and we are done.
  // That won't happen in this function, the code is asynchronous,
  // and broken over many functions, but the context is useful to
  // understand what is happening ...

  // ... we need to create an object to hold the client context,
  // status and result of the operation ...
  auto op = make_async_op<etcdserverpb::RangeResponse>(
      [this](std::shared_ptr<range_predecessor_op> op) {
        this->on_range_request(op);
      });
  op->rpc = kv_client_->AsyncRange(&op->context, req, *queue_);
  op->rpc->Finish(&op->response, &op->status, op->tag());
}

etcdserverpb::TxnResponse leader_election_participant::publish_value(
    std::string const& new_value, etcdserverpb::RequestOp const& failure_op) {
  etcdserverpb::TxnRequest req;
  auto& cmp = *req.add_compare();
  cmp.set_key(key());
  cmp.set_result(etcdserverpb::Compare::EQUAL);
  cmp.set_target(etcdserverpb::Compare::CREATE);
  cmp.set_create_revision(participant_revision_);
  auto& on_success = *req.add_success()->mutable_request_put();
  on_success.set_key(key());
  on_success.set_value(new_value);
  on_success.set_lease(lease_id());
  *req.add_failure() = failure_op;

  return commit(req);
}

etcdserverpb::TxnResponse
leader_election_participant::commit(etcdserverpb::TxnRequest const& req) {
  grpc::ClientContext context;
  etcdserverpb::TxnResponse resp;
  auto status = kv_client_->Txn(&context, req, &resp);
  if (not status.ok()) {
    throw error_grpc_status("commit/etcd::Txn", status, nullptr, &req);
  }
  return resp;
}

void leader_election_participant::on_writes_done(
    std::shared_ptr<writes_done_op> writes_done, std::promise<bool>& done) {
  auto op = make_finish_op([this, &done](auto op) { on_finish(op, done); });
  watcher_stream_->Finish(&op->status, op->tag());
}

void leader_election_participant::on_finish(
    std::shared_ptr<finish_op> op, std::promise<bool>& done) {
  try {
    if (not op->status.ok()) {
      throw error_grpc_status("on_finish", op->status);
    }
    done.set_value(true);
  } catch (...) {
    try {
      done.set_exception(std::current_exception());
    } catch (std::future_error const& ex) {
      JB_LOG(info) << "std::future_error raised while reporting "
                   << "exception in watcher stream closure."
                   << "  participant=" << key() << ", value=" << value()
                   << ", exception=" << ex.what();
    } catch (std::exception const& ex) {
      JB_LOG(info) << "std::exception raised while reporting "
                   << "exception in watcher stream closure."
                   << "  participant=" << key() << ", value=" << value()
                   << ", exception=" << ex.what();
    } catch (...) {
      JB_LOG(info) << "std::exception raised while reporting "
                   << "exception in watcher stream closure."
                   << "  participant=" << key() << ", value=" << value();
    }
  }
}

void leader_election_participant::on_range_request(
    std::shared_ptr<range_predecessor_op> op) {
  if (not op->status.ok()) {
    throw error_grpc_status("on_range_request", op->status, &op->response);
  }
  using etcdserverpb::WatchResponse;
  using etcdserverpb::WatchRequest;
  for (auto const& kv : op->response.kvs()) {
    // ... we need to capture the key and revision of the result, so
    // we can then start a Watch starting from that revision ...
    ++pending_watches_;

    auto write = make_write_op<etcdserverpb::WatchRequest>([
      this, key = kv.key(), revision = op->response.header().revision()
    ](auto op) { this->on_watch_write(op, key, revision); });
    auto& create = *write->request.mutable_create_request();
    create.set_key(kv.key());
    create.set_start_revision(op->response.header().revision());
    watcher_stream_->Write(write->request, op->tag());
  }
  check_election_over_maybe();
}

void leader_election_participant::on_watch_write(
    std::shared_ptr<watch_write_op> op, std::string const& key,
    std::uint64_t revision) {
  auto read = make_read_op<etcdserverpb::WatchResponse>([this, key, revision](
      auto op) { this->on_watch_read(op, key, revision); });
  watcher_stream_->Read(&read->response, op->tag());
}

void leader_election_participant::on_watch_read(
    std::shared_ptr<watch_read_op> op, std::string const& key,
    std::uint64_t revision) {
  for (auto const& ev : op->response.events()) {
    if (ev.type() != mvccpb::Event::DELETE) {
      continue;
    }
    --pending_watches_;
  }
  if (op->response.canceled()) {
    JB_LOG(info) << "Watcher unexpectedly canceled for key=" << key
                 << ", revision=" << revision
                 << ", reason=" << op->response.cancel_reason();
  }
  if (op->response.compact_revision()) {
    // TODO() - if I am reading the documentation correctly, this
    // means the watcher was cancelled.  We need to worry about the
    // case where the participant figures out the key to watch.  Then
    // it goes to sleep, or gets reschedule, then the key is deleted
    // and etcd compacted.  And then the client starts watching.  I am
    // not sure this is a problem, but it might be.
    JB_LOG(info) << "Watcher has compact_revision="
                 << op->response.compact_revision() << ", key=" << key
                 << ", revision=" << revision
                 << ", reason=" << op->response.cancel_reason();
  }
  check_election_over_maybe();
}

void leader_election_participant::check_election_over_maybe() {
  // ... check the atomic, do not worry about changes without a lock,
  // if it is positive then a future Read() will decrement it and we
  // will check again ...
  if (pending_watches_.load() != 0) {
    return;
  }
  campaign_result_.set_value(true);
  make_callback();
}

void leader_election_participant::run() {
  // You would think this is easy.  We need to make sure the callbacks
  // and futures are satisfied correctly when the thread exits, even
  // if an exception is raised ..
  try {
    queue_->run();
  } catch (...) {
    auto eptr = std::current_exception();
    log_thread_exit_exception(eptr);
    try {
      campaign_result_.set_exception(eptr);
    } catch (std::future_error const& ex) {
      // ... this is normal, the campaign was already satisfied ...
      JB_LOG(debug) << "std::future_error raised while reporting "
                    << "exception at event loop exit."
                    << "  participant=" << key() << ", value=" << value()
                    << ", exception=" << ex.what();
    } catch (std::exception const& ex) {
      // ... this is not normal, report the exception ...
      JB_LOG(debug) << "Standard C++ exception raised while reporting "
                    << "exception at event loop exit."
                    << "  participant=" << key() << ", value=" << value()
                    << ", exception=" << ex.what();
    } catch (...) {
      // ... this is not normal, report the exception ...
      JB_LOG(debug) << "Unknown exception raised while reporting "
                    << "exception at event loop exit."
                    << "  participant=" << key() << ", value=" << value();
    }
  }
  // TODO() - maybe we should use make_ready_at_thread_exit() or some
  // of
  make_callback();
}

void leader_election_participant::make_callback() {
  std::function<void(std::future<bool>&)> callback;
  {
    std::lock_guard<std::mutex> lock(mu_);
    callback = std::move(campaign_callback_);
    if (not callback) {
      return;
    }
  }
  auto future = campaign_result_.get_future();
  callback(future);
}

void leader_election_participant::log_thread_exit_exception(
    std::exception_ptr eptr) {
  try {
    if (eptr) {
      std::rethrow_exception(eptr);
    } else {
      JB_LOG(info) << "Exception raised at event loop exit,"
                   << " but got null exception pointer."
                   << "  participant=" << key() << ", value=" << value();
    }
  } catch (std::exception const& ex) {
    JB_LOG(info) << "Standard C++ exception raised at event loop exit."
                 << "  participant=" << key() << ", value=" << value()
                 << ", exception=" << ex.what();
  } catch (...) {
    JB_LOG(info) << "Unknown C++ exception raised at event loop exit."
                 << "  participant=" << key() << ", value=" << value();
  }
}

std::runtime_error leader_election_participant::error_grpc_status(
    char const* where, grpc::Status const& status,
    google::protobuf::Message const* res,
    google::protobuf::Message const* req) const {
  std::ostringstream os;
  os << "grpc error in " << where << " for election=" << election_name_
     << ", participant=" << key() << ", value=" << value()
     << ", revision=" << participant_revision_ << ": " << status.error_message()
     << "[" << status.error_code() << "]";
  if (res != nullptr) {
    std::string print;
    google::protobuf::TextFormat::PrintToString(*res, &print);
    os << ", response=" << print;
  }
  if (req != nullptr) {
    std::string print;
    google::protobuf::TextFormat::PrintToString(*req, &print);
    os << ", request=" << print;
  }
  return std::runtime_error(os.str());
}

std::string leader_election_participant::prefix_end(std::string const& prefix) {
  // ... iterate in reverse order, find the first element (in
  // iteration order) that is not 0xFF, increment it, now we have a
  // key that is one bit higher than the prefix ...
  std::string range_end = prefix;
  bool needs_append = true;
  for (auto i = range_end.rbegin(); i != range_end.rend(); ++i) {
    if (std::uint8_t(*i) == 0xFF) {
      *i = 0x00;
      continue;
    }
    *i = *i + 1;
    needs_append = false;
    break;
  }
  // ... if all elements were 0xFF the loop converted them to 0x00 and
  // we just need to add a 0x01 at the end ...
  if (needs_append) {
    range_end.push_back(0x01);
  }
  return range_end;
}

} // namespace etcd
} // namespace jb
