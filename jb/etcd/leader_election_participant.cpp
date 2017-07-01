#include "jb/etcd/leader_election_participant.hpp"
#include <jb/etcd/grpc_errors.hpp>
#include <jb/etcd/log_promise_errors.hpp>
#include <jb/assert_throw.hpp>
#include <jb/log.hpp>

#include <google/protobuf/text_format.h>
#include <iomanip>
#include <sstream>
#include <stdexcept>

namespace jb {
namespace etcd {

static char const* str(leader_election_participant::state s) {
  static char const* names[] = {
      "constructing", "connecting", "testandset",   "republish",
      "published",    "querying",   "campaigning",  "elected",
      "resigning",    "resigned",   "shuttingdown", "shutdown",
  };
  if (int(s) < 0 or std::size_t(s) >= sizeof(names) / sizeof(names[0])) {
    return "[--invalid--]";
  }
  return names[int(s)];
}

// Notice that this constructor does not kick-off the campaign, it is
// a private constructor, it captures the shared code between the
// other constructors.  Each one of them kicks off the campaign is a
// slightly different way ...
leader_election_participant::leader_election_participant(
    bool shared, std::shared_ptr<active_completion_queue> queue,
    std::shared_ptr<grpc::Channel> etcd_channel,
    std::string const& election_name, std::string const& participant_value,
    std::shared_ptr<session> session)
    : queue_(queue)
    , channel_(etcd_channel)
    , session_(session)
    , kv_client_(etcdserverpb::KV::NewStub(channel_))
    , watch_client_(etcdserverpb::Watch::NewStub(channel_))
    , watcher_stream_()
    , election_name_(election_name)
    , participant_value_(participant_value)
    , election_prefix_(election_name + "/")
    , participant_key_([this]() {
      // ... build the key using the session's lease.  The lease id is
      // unique, assigned by etcd, so each participant gets a different
      // key.  The use of an inline functor is because why not ...
      // TODO() - document why there can be no accidental conflict with
      // another election running in the system.
      std::ostringstream os;
      os << election_prefix_ << std::hex << this->session_->lease_id();
      return os.str();
    }())
    , mu_()
    , cv_()
    , state_(state::constructing)
    , current_watches_()
    , watched_keys_()
    , pending_async_ops_(0)
    , campaign_result_()
    , campaign_callback_() {
  // ... this is just the initialization after all the member
  // variables are set.  Moved to a function because the code gets too
  // big otherwise ...
  preamble();
}

leader_election_participant::~leader_election_participant() noexcept(false) {
  shutdown();
}

void leader_election_participant::resign() {
  set_state("resign() begin", state::resigning);
  // ... this blocks until the lease is removed, at that point the
  // etcd resources are all deleted, except for the watchers ...
  session_->revoke();
  std::set<std::uint64_t> watches;
  {
    std::lock_guard<std::mutex> lock(mu_);
    if (state_ != state::resigning) {
      JB_LOG(trace) << key() << " " << str(state_) << " " << pending_async_ops_
                    << " unexpected state for resign()/watches";
      std::ostringstream os;
      os << key() << " unexpected state " << str(state_) << " while resigning";
      throw std::runtime_error(os.str());
    }
    watches = std::move(current_watches_);
  }
  // ... cancel all the watchers too ...
  for (auto w : watches) {
    JB_LOG(trace) << key() << " " << str(state_) << " " << pending_async_ops_
                  << "  cancel watch = " << w;
    if (not async_op_start("cancel watch")) {
      return;
    }
    etcdserverpb::WatchRequest req;
    auto& cancel = *req.mutable_cancel_request();
    cancel.set_watch_id(w);

    queue_->cq().async_write(
        watcher_stream_, std::move(req),
        "leader_election_participant/resign/cancel",
        [this, w](auto op, bool ok) { this->on_watch_cancel(op, ok, w); });
  }
  async_ops_block();
  // ... now we are really done with remote resources ...
  set_state("resign() end", state::resigning);
}

void leader_election_participant::proclaim(std::string const& new_value) {
  JB_LOG(trace) << key() << " " << str(state_) << " " << pending_async_ops_
                << " proclaim(" << new_value << ")";
  std::string copy(new_value);
  etcdserverpb::RequestOp failure_op;
  auto result = publish_value(copy, failure_op);
  if (result.succeeded()) {
    copy.swap(participant_value_);
    JB_LOG(trace) << key() << " " << str(state_) << " " << pending_async_ops_
                  << " proclaim(" << new_value << ") - success";
    return;
  }
  std::ostringstream os;
  os << key() << " unexpected failure writing new value: \n";
  std::string print;
  google::protobuf::TextFormat::PrintToString(result, &print);
  os << "txn result=" << print << "\n";
  throw std::runtime_error(os.str());
}

void leader_election_participant::preamble() try {
  // ... no real need to grab a mutex here.  The object is not fully
  // constructed, it should not be used by more than one thread ...
  set_state("preamble()", state::connecting);

  async_op_start("create stream / preamble/lambda()");
  std::promise<bool> stream_ready;
  queue_->cq().async_create_rdwr_stream(
      watch_client_.get(), &etcdserverpb::Watch::Stub::AsyncWatch,
      "leader_election_participant/watch",
      [this, &stream_ready](auto stream, bool ok) {
        this->async_op_done("create stream / preamble/lambda()");
        if (ok) {
          this->watcher_stream_ = std::move(stream);
        }
        stream_ready.set_value(ok);
      });

  // ... blocks until it is ready ...
  if (not stream_ready.get_future().get()) {
    JB_LOG(error) << key() << " " << str(state_) << " " << pending_async_ops_
                  << "  stream not ready!!";
    throw std::runtime_error("cannot complete watcher stream setup");
  }

  set_state("preamble()", state::testandset);

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
  {
    std::string tmp;
    google::protobuf::TextFormat::PrintToString(resp, &tmp);
    JB_LOG(trace) << key() << " " << str(state_) << " " << pending_async_ops_
                  << "  commit() with resp=" << tmp;
  }

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
      set_state("preamble()", state::republish);
      // ... too bad, need to publish again *and* we need to delete
      // the key if the publication fails ...
      etcdserverpb::RequestOp failure_op;
      auto& delete_op = *failure_op.mutable_request_delete_range();
      delete_op.set_key(key());
      auto published = publish_value(value(), failure_op);
      {
        std::string tmp;
        google::protobuf::TextFormat::PrintToString(published, &tmp);
        JB_LOG(trace) << key() << " " << str(state_) << " "
                      << pending_async_ops_
                      << "  published_value() with resp=" << tmp;
      }
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
  set_state("preamble()", state::published);
} catch (std::exception const& ex) {
  JB_LOG(info) << key() << " " << str(state_) << " " << pending_async_ops_
               << " std::exception raised in preamble: " << ex.what();
  shutdown();
  throw;
} catch (...) {
  JB_LOG(info) << key() << " " << str(state_) << " " << pending_async_ops_
               << " unknown exception raised in preamble";
  shutdown();
  throw;
}

void leader_election_participant::shutdown() {
  if (not set_state("shutdown()", state::shuttingdown)) {
    return;
  }
  JB_LOG(trace) << key() << " " << str(state_) << " " << pending_async_ops_
                << "  shutdown";
  // ... if there is a pending range request we need to block on it ...
  async_ops_block();
  if (watcher_stream_) {
    // The watcher stream was already created, we need to close it
    // before shutting down the completion queue ...
    std::promise<bool> stream_closed;
    (void)async_op_start_shutdown("writes done");
    queue_->cq().async_writes_done(
        watcher_stream_, "leader_election_participant/shutdown/writes_done",
        [this, &stream_closed](auto op, bool ok) {
          this->on_writes_done(op, ok, stream_closed);
        });
    // ... block until it closes ...
    bool success = stream_closed.get_future().get();
    if (success) {
      JB_LOG(trace) << key() << " " << str(state_) << " " << pending_async_ops_
                    << "  writes done completed";
    }

    std::promise<bool> stream_finished;
    (void)async_op_start_shutdown("finish");
    queue_->cq().async_finish(
        watcher_stream_, "leader_election_participant/shutdown/finish",
        [this, &stream_finished](auto const& op, bool ok) {
          this->on_finish(op, ok, stream_finished);
        });
    JB_LOG(trace) << key() << " " << str(state_) << " " << pending_async_ops_
                  << "  finish scheduled";
    // ... block until it closes ...
    auto fut = stream_finished.get_future();
    // TODO() - this is a workaround, the finish() call does not seem
    // to terminate for me ...
    if (fut.wait_for(std::chrono::milliseconds(200)) ==
        std::future_status::timeout) {
      JB_LOG(info) << key() << " " << str(state_) << " " << pending_async_ops_
                   << "  timeout on Finish() call, forcing a on_finish()";
      async_op_done("on_finish() - forced");
      try {
        stream_finished.set_value(false);
      } catch(...) {
        log_promise_errors(
            stream_finished, std::current_exception(),
            log_header("on_finish() - forced"), "");
      }
    }
  }
  (void)set_state("shutdown()", state::shutdown);
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
  JB_LOG(trace) << key() << " " << str(state_) << " " << pending_async_ops_
                << "  blocked running election";
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
  JB_LOG(trace) << key() << " " << str(state_) << " " << pending_async_ops_
                << "  kicking off campaign";
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
  //   - Search all the keys that have the same prefix (that is the
  //     election_prefix_
  req.set_key(election_prefix_);
  //   - Prefix searches are range searches where the end value is 1
  //     bit higher than the initial value.
  req.set_range_end(prefix_end(election_prefix_));
  //   - Limit those results to the keys that have creation_revision
  //     lower than this participant's creation_revision key
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
  std::string payload;
  google::protobuf::TextFormat::PrintToString(req, &payload);
  JB_LOG(trace) << key() << " " << str(state_) << " " << pending_async_ops_
                << " range request(), rev=" << participant_revision_ << "\n"
                << payload;
  (void)set_state("campaign_impl()", state::querying);
  async_op_start("range request");
  queue_->cq().async_rpc(
      kv_client_.get(), &etcdserverpb::KV::Stub::AsyncRange, std::move(req),
      "leader_election_participant/campaign/range",
      [this](auto const& op, bool ok) { this->on_range_request(op, ok); });
}

etcdserverpb::TxnResponse leader_election_participant::publish_value(
    std::string const& new_value, etcdserverpb::RequestOp const& failure_op) {
  JB_LOG(trace) << key() << " " << str(state_) << " " << pending_async_ops_
                << " publish_value()";
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
  if (failure_op.request_case() != etcdserverpb::RequestOp::REQUEST_NOT_SET) {
    *req.add_failure() = failure_op;
  }

  return commit(req);
}

etcdserverpb::TxnResponse
leader_election_participant::commit(etcdserverpb::TxnRequest const& req) {
  grpc::ClientContext context;
  etcdserverpb::TxnResponse resp;
  auto status = kv_client_->Txn(&context, req, &resp);
  check_grpc_status(
      status, log_header("commit/etcd::Txn"), ", request=",
      print_to_stream(req));
  return resp;
}

void leader_election_participant::on_range_request(
    detail::async_op<
        etcdserverpb::RangeRequest, etcdserverpb::RangeResponse> const& op,
    bool ok) try {
  async_op_done("on_range_request()");
  check_grpc_status(
      op.status, log_header("on_range_request()"), ", response=",
      print_to_stream(op.response));

  for (auto const& kv : op.response.kvs()) {
    // ... we need to capture the key and revision of the result, so
    // we can then start a Watch starting from that revision ...

    if (not async_op_start("create watch")) {
      return;
    }
    (void)set_state("on_range_request()", state::campaigning);
    JB_LOG(trace) << key() << " " << str(state_) << " " << pending_async_ops_
                  << "  create watcher ... k=" << kv.key();
    watched_keys_.insert(kv.key());

    etcdserverpb::WatchRequest req;
    auto& create = *req.mutable_create_request();
    create.set_key(kv.key());
    create.set_start_revision(op.response.header().revision() - 1);

    queue_->cq().async_write(
        watcher_stream_, std::move(req),
        "leader_election_participant/on_range_request/watch", [
          this, key = kv.key(), revision = op.response.header().revision()
        ](auto op, bool ok) { this->on_watch_create(op, ok, key, revision); });
  }
  check_election_over_maybe();
} catch (...) {
}

void leader_election_participant::on_watch_create(
    watch_write_op const& op, bool ok, std::string const& key,
    std::uint64_t revision) {
  async_op_done("on_watch_create()");
  if (not ok) {
    return;
  }
  if (not async_op_start("read watch")) {
    return;
  }

  queue_->cq().async_read(
      watcher_stream_, "leader_election_participant/on_watch_create/read",
      [this, key, revision](auto op, bool ok) {
        this->on_watch_read(op, ok, key, revision);
      });
}

void leader_election_participant::on_watch_cancel(
    watch_write_op const& op, bool, std::uint64_t) {
  // ... there should be a Read() pending already ...
  async_op_done("on_watch_cancel()");
}

void leader_election_participant::on_watch_read(
    watch_read_op const& op, bool ok, std::string const& wkey,
    std::uint64_t revision) {
  async_op_done("on_watch_read()");
  if (not ok) {
    JB_LOG(info) << key() << " " << str(state_) << " " << pending_async_ops_
                 << "  watcher called with ok=false key=" << wkey;
    return;
  }
  if (op.response.created()) {
    JB_LOG(trace) << "  received new watcher=" << op.response.watch_id();
    std::lock_guard<std::mutex> lock(mu_);
    current_watches_.insert(op.response.watch_id());
  } else {
    JB_LOG(trace) << key() << " " << str(state_) << " " << pending_async_ops_
                  << "  update for existing watcher=" << op.response.watch_id();
  }
  for (auto const& ev : op.response.events()) {
    // ... DELETE events indicate that the other participant's lease
    // expired, or they actively resigned, other events are not of
    // interest ...
    if (ev.type() != mvccpb::Event::DELETE) {
      continue;
    }
    // ... remove that key from the set of keys we are waiting to be
    // deleted ...
    watched_keys_.erase(ev.kv().key());
  }
  check_election_over_maybe();
  // ... unless the watcher was canceled we should continue to read
  // from it ...
  if (op.response.canceled()) {
    JB_LOG(trace) << key() << " " << str(state_) << " " << pending_async_ops_
                  << " watcher canceled for key=" << wkey
                  << ", revision=" << revision
                  << ", reason=" << op.response.cancel_reason()
                  << ", watch_id=" << op.response.watch_id();
    current_watches_.erase(op.response.watch_id());
    return;
  }
  if (op.response.compact_revision()) {
    // TODO() - if I am reading the documentation correctly, this
    // means the watcher was cancelled.  We need to worry about the
    // case where the participant figures out the key to watch.  Then
    // it goes to sleep, or gets reschedule, then the key is deleted
    // and etcd compacted.  And then the client starts watching.  I am
    // not sure this is a problem, but it might be.
    JB_LOG(trace) << key() << " " << str(state_) << " " << pending_async_ops_
                  << " watcher cancelled with compact_revision="
                  << op.response.compact_revision() << ", key=" << wkey
                  << ", revision=" << revision
                  << ", reason=" << op.response.cancel_reason()
                  << ", watch_id=" << op.response.watch_id();
    current_watches_.erase(op.response.watch_id());
    return;
  }
  {
    std::lock_guard<std::mutex> lock(mu_);
    if (state_ == state::shuttingdown or state_ == state::shutdown) {
      return;
    }
  }
  // ... the watcher was not canceled, so try reading again ...
  if (not async_op_start("read watch / followup")) {
    return;
  }

  queue_->cq().async_read(
      watcher_stream_, "leader_election_participant/on_watch_read/read",
      [this, wkey, revision](auto op, bool ok) {
        this->on_watch_read(op, ok, wkey, revision);
      });
}

void leader_election_participant::on_writes_done(
    detail::writes_done_op const& writes_done, bool ok,
    std::promise<bool>& done) {
  async_op_done("on_writes_done()");
  if (not ok) {
    // ... operation aborted, just signal and return ...
    done.set_value(ok);
    return;
  }
  done.set_value(true);
}

void leader_election_participant::on_finish(
    detail::finish_op const& op, bool ok, std::promise<bool>& done) {
  async_op_done("on_finish()");
  if (not ok) {
    // ... operation canceled, no sense in waiting anymore ...
    done.set_value(false);
    return;
  }
  try {
    check_grpc_status(op.status, log_header("on_finish()"));
    done.set_value(true);
  } catch (...) {
    log_promise_errors(
        done, std::current_exception(), log_header("on_finish()"), "");
  }
}

void leader_election_participant::check_election_over_maybe() {
  // ... check the atomic, do not worry about changes without a lock,
  // if it is positive then a future Read() will decrement it and we
  // will check again ...
  {
    std::lock_guard<std::mutex> lock(mu_);
    if (not current_watches_.empty() or not watched_keys_.empty()) {
      return;
    }
    if (state_ != state::shuttingdown and state_ != state::shutdown) {
      state_ = state::elected;
    }
  }
  JB_LOG(trace) << key() << " " << str(state_) << " " << pending_async_ops_
                << " election completed";
  campaign_result_.set_value(true);
  make_callback();
}

void leader_election_participant::make_callback() {
  std::function<void(std::future<bool>&)> callback;
  {
    std::lock_guard<std::mutex> lock(mu_);
    callback = std::move(campaign_callback_);
    if (not callback) {
      JB_LOG(info) << key() << " " << str(state_) << " " << pending_async_ops_
                   << " no callback present";
      return;
    }
  }
  auto future = campaign_result_.get_future();
  callback(future);
  JB_LOG(trace) << key() << " " << str(state_) << " " << pending_async_ops_
                << "  made callback";
}

std::string leader_election_participant::log_header(char const* loc) const {
  std::ostringstream os;
  os << key() << " " << str(state_) << " " << pending_async_ops_ << loc;
  return os.str();
}

void leader_election_participant::async_ops_block() {
  std::unique_lock<std::mutex> lock(mu_);
  cv_.wait(lock, [this]() { return pending_async_ops_ == 0; });
}

bool leader_election_participant::async_op_start(char const* msg) {
  JB_LOG(trace) << key() << " " << str(state_) << " " << pending_async_ops_
                << "      " << msg;
  std::unique_lock<std::mutex> lock(mu_);
  if (state_ == state::shuttingdown or state_ == state::shutdown) {
    return false;
  }
  ++pending_async_ops_;
  return true;
}

bool leader_election_participant::async_op_start_shutdown(char const* msg) {
  JB_LOG(trace) << key() << " " << str(state_) << " " << pending_async_ops_
                << "      " << msg << " during shutdown";
  std::unique_lock<std::mutex> lock(mu_);
  ++pending_async_ops_;
  return true;
}

void leader_election_participant::async_op_done(char const* msg) {
  JB_LOG(trace) << key() << " " << str(state_) << " " << pending_async_ops_
                << "      " << msg;
  std::unique_lock<std::mutex> lock(mu_);
  if (--pending_async_ops_ == 0) {
    lock.unlock();
    cv_.notify_one();
    return;
  }
}

bool leader_election_participant::set_state(char const* msg, state new_state) {
  std::lock_guard<std::mutex> lock(mu_);
  JB_LOG(trace) << key() << " " << str(state_) << " " << pending_async_ops_
                << "      " << msg << " " << str(new_state);
  if (state_ == state::shuttingdown or state_ == state::shutdown) {
    return false;
  }
  state_ = new_state;
  return true;
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
