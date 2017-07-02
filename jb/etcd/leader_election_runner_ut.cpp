#include <jb/etcd/detail/mocked_grpc_interceptor.hpp>
#include <jb/etcd/grpc_errors.hpp>
#include <jb/etcd/leader_election_participant.hpp>
#include <jb/etcd/prefix_end.hpp>
#include <jb/testing/future_status.hpp>
#include <jb/assert_throw.hpp>
#include <jb/log.hpp>

#include <boost/test/unit_test.hpp>

#include <google/protobuf/text_format.h>
#include <sstream>
#include <stdexcept>

namespace jb {
namespace etcd {
  //@{
  using watcher_stream_type = detail::async_rdwr_stream<
      etcdserverpb::WatchRequest, etcdserverpb::WatchResponse>;
  using watch_write_op = watcher_stream_type::write_op;
  using watch_read_op = watcher_stream_type::read_op;
  //@}

  /**
   * The implicit state machine in a leader election participant.
   *
   * Most of the states are there for debugging, the state machine is
   * implicit after all.  Only shuttingdown and shutdown are used to
   * stop new async operations from starting.
   *
   * -# constructing: the initial state.
   * -# connecting: setting up bi-direction stream for watchers.
   * -# testandtest:
   * -# republish:
   * -# published:
   * -# querying: discover the previous participant in the election.
   * -# campaigning: the previous participant is known and the
   *    watchers are setup
   * -# elected
   * -# resigning
   * -# shuttingdown
   * -# shutdown
   *
   */
  enum class leader_election_state {
    constructing,
    connecting,
    testandset,
    republish,
    published,
    querying,
    campaigning,
    elected,
    resigning,
    resigned,
    shuttingdown,
    shutdown,
  };

static char const* str(leader_election_state s) {
  static char const* names[] = {
      "constructing", "connecting", "testandset",   "republish",
      "published",    "querying",   "campaigning",  "elected",
      "resigning",    "resigned",   "shuttingdown", "shutdown",
  };
  JB_ASSERT_THROW(
      0 <= int(s) and std::size_t(s) < sizeof(names) / sizeof(names[0]));
  return names[int(s)];
}

/**
 * Participate in a leader election protocol.
 */
class leader_election_runner_base {
public:
  // Only derived classes have access to the constructor

  /// Destructor
  virtual ~leader_election_runner_base() noexcept(false) {
  }

  using state = leader_election_state;

  /// Return the etcd key associated with this participant
  std::string const& key() const {
    return participant_key_;
  }

  /// Return the etcd eky associated with this participant
  std::string const& value() const {
    return participant_value_;
  }

  /// Return the lease corresponding to this participant's session.
  std::uint64_t lease_id() {
    return lease_id_;
  }

  /// Return a nice header for all log and error messages.
  std::string log_header(char const* loc) const {
    std::ostringstream os;
    os << key() << " " << str(state_) << " " << pending_async_ops_ << loc;
    return os.str();
  }

  void async_ops_block() {
    std::unique_lock<std::mutex> lock(mu_);
    cv_.wait(lock, [this]() { return pending_async_ops_ == 0; });
  }

  bool async_op_start(char const* msg) {
    JB_LOG(trace) << key() << " " << str(state_) << " " << pending_async_ops_
                  << "      " << msg;
    std::unique_lock<std::mutex> lock(mu_);
    if (state_ == state::shuttingdown or state_ == state::shutdown) {
      return false;
    }
    ++pending_async_ops_;
    return true;
  }

  bool async_op_start_shutdown(char const* msg) {
    JB_LOG(trace) << key() << " " << str(state_) << " " << pending_async_ops_
                  << "      " << msg << " during shutdown";
    std::unique_lock<std::mutex> lock(mu_);
    ++pending_async_ops_;
    return true;
  }

  void async_op_done(char const* msg) {
    JB_LOG(trace) << key() << " " << str(state_) << " " << pending_async_ops_
                  << "      " << msg;
    std::unique_lock<std::mutex> lock(mu_);
    if (--pending_async_ops_ == 0) {
      lock.unlock();
      cv_.notify_one();
      return;
    }
  }

  bool set_state(char const* msg, state new_state) {
    std::lock_guard<std::mutex> lock(mu_);
    JB_LOG(trace) << key() << " " << str(state_) << " " << pending_async_ops_
                  << "      " << msg << " " << str(new_state);
    if (state_ == state::shuttingdown or state_ == state::shutdown) {
      return false;
    }
    state_ = new_state;
    return true;
  }

protected:
  leader_election_runner_base(
      std::uint64_t lease_id, std::unique_ptr<etcdserverpb::KV::Stub> kv_client,
      std::unique_ptr<etcdserverpb::Watch::Stub> watch_client,
      std::string const& election_name, std::string const& participant_value)
      : mu_()
      , cv_()
      , state_(state::constructing)
      , pending_async_ops_(0)
      , election_name_(election_name)
      , participant_value_(participant_value)
      , election_prefix_(election_name + "/")
      , participant_key_([election_name, lease_id]() {
        // ... build the key using the session's lease.  The lease id is
        // unique, assigned by etcd, so each participant gets a different
        // key.  The use of an inline functor is because why not ...
        // TODO() - document why there can be no accidental conflict with
        // another election running in the system.
        std::ostringstream os;
        os << election_name << "/" << std::hex << lease_id;
        return os.str();
      }())
      , lease_id_(lease_id)
      , kv_client_(std::move(kv_client))
      , watch_client_(std::move(watch_client))
      , watcher_stream_() {
  }


protected:
  mutable std::mutex mu_;
  std::condition_variable cv_;
  state state_;
  int pending_async_ops_;

  std::string election_name_;
  std::string participant_value_;
  std::string election_prefix_;
  std::string participant_key_;
  std::uint64_t lease_id_;

  std::unique_ptr<etcdserverpb::KV::Stub> kv_client_;
  std::unique_ptr<etcdserverpb::Watch::Stub> watch_client_;
  std::unique_ptr<watcher_stream_type> watcher_stream_;
};

/**
 * Participate in a leader election protocol.
 */
template <typename completion_queue_type>
class leader_election_runner : public leader_election_runner_base {
public:
  /// Constructor, non-blocking, calls the callback when elected.
  template <typename Functor>
  leader_election_runner(
      completion_queue_type& queue, std::uint64_t lease_id,
      std::unique_ptr<etcdserverpb::KV::Stub> kv_client,
      std::unique_ptr<etcdserverpb::Watch::Stub> watch_client,
      std::string const& election_name, std::string const& participant_value,
      Functor&& elected_callback)
      : leader_election_runner_base(
            lease_id, std::move(kv_client), std::move(watch_client),
            election_name, participant_value)
      , queue_(queue)
      , participant_revision_(0)
      , current_watches_()
      , watched_keys_()
      , campaign_result_()
      , campaign_callback_() {
    preamble();
    campaign(elected_callback);
  }

  /**
   * Release local resources.
   *
   * The destructor makes sure the *local* resources are released,
   * including connections to the etcd server, and pending
   * operations.  It makes no attempt to resign from the election, or
   * delete the keys in etcd, or to gracefully revoke the etcd leases.
   *
   * The application should call resign() to release the resources
   * held in the etcd server *before* the destructor is called.
   */
  ~leader_election_runner() noexcept(false) {
    shutdown();
  }

  /// Resign from the election, terminate the internal loops
  void resign() {
    set_state("resign() begin", state::resigning);
    std::set<std::uint64_t> watches;
    {
      std::lock_guard<std::mutex> lock(mu_);
      if (state_ != state::resigning) {
        JB_LOG(trace) << key() << " " << str(state_) << " "
                      << pending_async_ops_
                      << " unexpected state for resign()/watches";
        std::ostringstream os;
        os << key() << " unexpected state " << str(state_)
           << " while resigning";
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

      queue_.async_write(
          watcher_stream_, std::move(req),
          "leader_election_participant/resign/cancel",
          [this, w](auto op, bool ok) { this->on_watch_cancel(op, ok, w); });
    }
    async_ops_block();
    // ... now we are really done with remote resources ...
    set_state("resign() end", state::resigning);
  }

  /// Change the published value
  void proclaim(std::string const& new_value) {
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

  /**
   * Runs the operations before starting the election campaign.
   *
   * This function can throw exceptions which means the campaign was
   * never even started.
   */
  void preamble() try {
    // ... no real need to grab a mutex here.  The object is not fully
    // constructed, it should not be used by more than one thread ...
    set_state("preamble()", state::connecting);

    async_op_start("create stream / preamble/lambda()");
    std::promise<bool> stream_ready;
    queue_.async_create_rdwr_stream(
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
    etcdserverpb::TxnResponse resp =
        commit(req, "leader_election/commit/create_node");
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

  /**
   * Gracefully shutdown a partially or fully constructed instance
   *
   * After the thread running the event loop is launched the
   * destruction process for this class is complicated.  The thread
   * must exit, or std::thread will call std::terminate because it was
   * not joined.
   * To terminate the thread we need to finish the completion queue
   * loop.
   * That requires terminating any pending operations, or the
   * completion queue calls abort().
   */
  void shutdown() {
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
      (void)async_op_start_shutdown("writes done");
      auto writes_done_complete = queue_.async_writes_done(
          watcher_stream_, "leader_election_participant/shutdown/writes_done",
          jb::etcd::use_future());

      // ... block until it closes ...
      writes_done_complete.get();
      JB_LOG(trace) << key() << " " << str(state_) << " " << pending_async_ops_
                    << "  writes done completed";

      (void)async_op_start_shutdown("finish");
      auto finished_complete = queue_.async_finish(
          watcher_stream_, "leader_election_participant/shutdown/finish",
          jb::etcd::use_future());
      JB_LOG(trace) << key() << " " << str(state_) << " " << pending_async_ops_
                    << "  finish scheduled";
      // TODO() - this is a workaround, the finish() call does not seem
      // to terminate for me ...
      if (finished_complete.wait_for(std::chrono::milliseconds(200)) ==
          std::future_status::timeout) {
        JB_LOG(info) << key() << " " << str(state_) << " " << pending_async_ops_
                     << "  timeout on Finish() call, forcing a on_finish()";
        async_op_done("on_finish() - forced");
      }
    }
    (void)set_state("shutdown()", state::shutdown);
  }

  /// Kick-off a campaign and call a functor when elected
  template <typename Functor>
  void campaign(Functor&& callback) {
    campaign_impl(std::function<void(std::future<bool>&)>(std::move(callback)));
  }

  /**
   * Refactor template code via std::function
   *
   * The main "interface" is the campaing() template member functions,
   * but we loath duplicating that much code here, so refactor with a
   * std::function<>.  The cost of such functions is higher, but
   * leader election is not a fast operation.
   */
  void campaign_impl(std::function<void(std::future<bool>&)>&& callback) {
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
    queue_.async_rpc(
        kv_client_.get(), &etcdserverpb::KV::Stub::AsyncRange, std::move(req),
        "leader_election_participant/campaign/range",
        [this](auto const& op, bool ok) { this->on_range_request(op, ok); });
  }

  /// Refactor code common to proclaim() and preamble()
  etcdserverpb::TxnResponse publish_value(
      std::string const& value, etcdserverpb::RequestOp const& failure_op) {
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
    on_success.set_value(value);
    on_success.set_lease(lease_id());
    if (failure_op.request_case() != etcdserverpb::RequestOp::REQUEST_NOT_SET) {
      *req.add_failure() = failure_op;
    }

    return commit(req, "leader_election/publish_value");
  }

  /// Refactor code to perform a Txn() request.
  etcdserverpb::TxnResponse
  commit(etcdserverpb::TxnRequest const& r, std::string name) {
    etcdserverpb::TxnRequest req = r;
    auto fut = queue_.async_rpc(
        kv_client_.get(), &etcdserverpb::KV::Stub::AsyncTxn, std::move(req),
        std::move(name), jb::etcd::use_future());
    return fut.get();
  }

  /// Called when the Range() operation in the kv_client completes.
  void on_range_request(
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

      queue_.async_write(
          watcher_stream_, std::move(req),
          "leader_election_participant/on_range_request/watch",
          [ this, key = kv.key(),
            revision = op.response.header().revision() ](auto op, bool ok) {
            this->on_watch_create(op, ok, key, revision);
          });
    }
    check_election_over_maybe();
  } catch (...) {
  }

  /// Called when a Write() operation that creates a watcher completes.
  void on_watch_create(
      watch_write_op const& op, bool ok, std::string const& watched_key,
      std::uint64_t watched_revision) {
    async_op_done("on_watch_create()");
    if (not ok) {
      return;
    }
    if (not async_op_start("read watch")) {
      return;
    }

    queue_.async_read(
        watcher_stream_, "leader_election_participant/on_watch_create/read",
        [this, watched_key, watched_revision](auto op, bool ok) {
          this->on_watch_read(op, ok, watched_key, watched_revision);
        });
  }

  /// Called when a Write() operation that cancels a watcher completes.
  void
  on_watch_cancel(watch_write_op const& op, bool ok, std::uint64_t watched_id) {
    // ... there should be a Read() pending already ...
    async_op_done("on_watch_cancel()");
  }

  /// Called when a Read() operation in the watcher stream completes.
  void on_watch_read(
      watch_read_op const& op, bool ok, std::string const& watched_key,
      std::uint64_t watched_revision) {
    async_op_done("on_watch_read()");
    if (not ok) {
      JB_LOG(info) << key() << " " << str(state_) << " " << pending_async_ops_
                   << "  watcher called with ok=false key=" << watched_key;
      return;
    }
    if (op.response.created()) {
      JB_LOG(trace) << "  received new watcher=" << op.response.watch_id();
      std::lock_guard<std::mutex> lock(mu_);
      current_watches_.insert(op.response.watch_id());
    } else {
      JB_LOG(trace) << key() << " " << str(state_) << " " << pending_async_ops_
                    << "  update for existing watcher="
                    << op.response.watch_id();
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
                    << " watcher canceled for key=" << watched_key
                    << ", revision=" << watched_revision
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
                    << op.response.compact_revision() << ", key=" << watched_key
                    << ", revision=" << watched_revision
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

    queue_.async_read(
        watcher_stream_, "leader_election_participant/on_watch_read/read",
        [this, watched_key, watched_revision](auto op, bool ok) {
          this->on_watch_read(op, ok, watched_key, watched_revision);
        });
  }

  /// Check if the election has finished, if so invoke the callbacks.
  void check_election_over_maybe() {
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

  // Invoke the callback, notice that the callback is invoked only once.
  void make_callback() {
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

private:
  completion_queue_type& queue_;
  std::uint64_t participant_revision_;

  std::set<std::uint64_t> current_watches_;
  std::set<std::string> watched_keys_;

  std::promise<bool> campaign_result_;
  std::function<void(std::future<bool>&)> campaign_callback_;
};

} // namespace etcd
} // namespace jb

/**
 * @test Verify that jb::etcd::leader_election_runner works in the
 * simple case.
 */
BOOST_AUTO_TEST_CASE(leader_election_runner_basic) {
  using namespace std::chrono_literals;

  // Create a null lease object, we do not need (or want) a real
  // connection for mocked operations ...
  std::shared_ptr<etcdserverpb::Lease::Stub> lease;

  using namespace jb::etcd;
  using completion_queue_type =
      completion_queue<detail::mocked_grpc_interceptor>;

  completion_queue_type queue;

  // The sequences of queries for a leader election participant is
  // rather long, but it goes like this ...

  using namespace ::testing;

  // ... on most calls we just invoke the application's callback
  // immediately ...
  EXPECT_CALL(*queue.interceptor().shared_mock, async_rpc(_))
      .WillRepeatedly(Invoke([](auto op) { op->callback(*op, true); }));
  EXPECT_CALL(*queue.interceptor().shared_mock, async_read(_))
      .WillRepeatedly(Invoke([](auto op) { op->callback(*op, true); }));
  EXPECT_CALL(*queue.interceptor().shared_mock, async_write(_))
      .WillRepeatedly(Invoke([](auto op) { op->callback(*op, true); }));
  EXPECT_CALL(*queue.interceptor().shared_mock, async_create_rdwr_stream(_))
      .WillRepeatedly(Invoke([](auto op) { op->callback(*op, true); }));
  EXPECT_CALL(*queue.interceptor().shared_mock, async_writes_done(_))
      .WillRepeatedly(Invoke([](auto op) { op->callback(*op, true); }));
  EXPECT_CALL(*queue.interceptor().shared_mock, async_finish(_))
      .WillRepeatedly(Invoke([](auto op) { op->callback(*op, true); }));

  // ... the class will try to create a node for the participant using
  // async_rpc, provide a good response for it ...
  EXPECT_CALL(*queue.interceptor().shared_mock, async_rpc(Truly([](auto op) {
    return op->name == "leader_election/commit/create_node";
  }))).WillOnce(Invoke([](auto bop) {
    using op_type =
        detail::async_op<etcdserverpb::TxnRequest, etcdserverpb::TxnResponse>;
    auto* op = dynamic_cast<op_type*>(bop.get());
    BOOST_REQUIRE(op != nullptr);
    // ... verify the request is what we expect ...
    BOOST_REQUIRE_EQUAL(op->request.compare().size(), 1UL);
    auto const& cmp = op->request.compare()[0];
    BOOST_CHECK_EQUAL(cmp.key(), "test-election/123456");
    BOOST_REQUIRE_EQUAL(op->request.success().size(), 1UL);
    auto const& success = op->request.success()[0];
    BOOST_CHECK_EQUAL(success.request_put().key(), "test-election/123456");
    BOOST_CHECK_EQUAL(success.request_put().value(), "mocked-runner-a");
    BOOST_CHECK_EQUAL(success.request_put().lease(), 0x123456);

    // ... in this test we just assume everything worked, so
    // provide a response ...
    op->response.set_succeeded(true);
    op->response.mutable_header()->set_revision(2345678UL);
    // ... and to not forget the callback ...
    bop->callback(*bop, true);
  }));

  using runner_type = leader_election_runner<completion_queue_type>;

  runner_type runner(
      queue, 0x123456UL, std::unique_ptr<etcdserverpb::KV::Stub>(),
      std::unique_ptr<etcdserverpb::Watch::Stub>(),
      std::string("test-election"),
      std::string("mocked-runner-a"), [](std::future<bool>&){});
}
