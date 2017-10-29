#ifndef jb_etcd_detail_leader_election_runner_impl_hpp
#define jb_etcd_detail_leader_election_runner_impl_hpp

#include <jb/etcd/completion_queue.hpp>
#include <jb/etcd/grpc_errors.hpp>
#include <jb/etcd/leader_election_runner.hpp>
#include <jb/etcd/prefix_end.hpp>
#include <jb/assert_throw.hpp>
#include <jb/log.hpp>

namespace jb {
namespace etcd {
namespace detail {

/**
 * Implement a leader election runner.
 */
template <typename completion_queue_type>
class leader_election_runner_impl : public leader_election_runner {
public:
  /// Constructor, non-blocking, calls the callback when elected.
  template <typename Functor>
  leader_election_runner_impl(
      completion_queue_type& queue, std::uint64_t lease_id,
      std::unique_ptr<etcdserverpb::KV::Stub> kv_client,
      std::unique_ptr<etcdserverpb::Watch::Stub> watch_client,
      std::string const& election_name, std::string const& participant_value,
      Functor&& elected_callback)
      : leader_election_runner(
            lease_id, std::move(kv_client), std::move(watch_client),
            election_name, participant_value)
      , queue_(queue)
      , current_watches_()
      , watched_keys_()
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
  ~leader_election_runner_impl() noexcept(false) {
    shutdown();
  }

  /// Resign from the election, terminate the internal loops
  void resign() override {
    std::set<std::uint64_t> watches;
    if (not set_state_action(
            "resign() begin", state::resigning,
            [&watches, this]() { watches = std::move(current_watches_); })) {
      return;
    }
    // ... cancel all the watchers too ...
    for (auto w : watches) {
      JB_LOG(trace) << log_header(" cancel watch") << " = " << w;
      if (not async_op_start("cancel watch")) {
        return;
      }
      etcdserverpb::WatchRequest req;
      auto& cancel = *req.mutable_cancel_request();
      cancel.set_watch_id(w);

      queue_.async_write(
          *watcher_stream_, std::move(req),
          "leader_election_participant/cancel_watcher",
          [this, w](auto op, bool ok) { this->on_watch_cancel(op, ok, w); });
    }
    // ... block until all pending operations complete ...
    async_ops_block();
    // ... if there is a pending callback we need to let them know the
    // election failed ...
    make_callback(false);
    // ... now we are really done with remote resources ...
    set_state("resign() end", state::resigned);
  }

  /// Change the published value
  void proclaim(std::string const& new_value) override {
    JB_LOG(trace) << log_header("") << " proclaim(" << new_value << ")";
    std::string copy(new_value);
    etcdserverpb::RequestOp failure_op;
    auto result = publish_value(copy, failure_op);
    if (result.succeeded()) {
      copy.swap(participant_value_);
      JB_LOG(trace) << log_header("") << " proclaim(" << new_value << ")";
      return;
    }
    std::ostringstream os;
    os << key() << " unexpected failure writing new value:\n"
       << print_to_stream(result) << "\n";
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

    auto fut = queue_.async_create_rdwr_stream(
        watch_client_.get(), &etcdserverpb::Watch::Stub::AsyncWatch,
        "leader_election_participant/watch", jb::etcd::use_future());
    watcher_stream_ = fut.get();
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
        if (not published.succeeded()) {
          // ... ugh, the publication failed.  We now have an
          // inconsistent state with the server.  We think we own the
          // code (and at least we own the lease!), but we were unable
          // to publish the value.  We are going to raise an exception
          // and abort the creation of the participant object ...
          std::ostringstream os;
          os << "Unexpected failure writing new value on existing key=" << key()
             << "\ntxn result=" << print_to_stream(published) << "\n";
          throw std::runtime_error(os.str());
        }
      }
    }
    set_state("preamble()", state::published);
  } catch (std::exception const& ex) {
    JB_LOG(trace) << log_header("")
                  << " std::exception raised in preamble: " << ex.what();
    shutdown();
    throw;
  } catch (...) {
    JB_LOG(trace) << log_header("") << " unknown exception raised in preamble";
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
    JB_LOG(trace) << log_header("") << "  shutdown";
    // ... if there is a pending range request we need to block on it ...
    async_ops_block();
    if (watcher_stream_) {
      // The watcher stream was already created, we need to close it
      // before shutting down the completion queue ...
      (void)async_op_start_shutdown("writes done");
      auto writes_done_complete = queue_.async_writes_done(
          *watcher_stream_, "leader_election_participant/shutdown/writes_done",
          jb::etcd::use_future());

      // ... block until it closes ...
      writes_done_complete.get();
      JB_LOG(trace) << log_header("") << "  writes done completed";

      (void)async_op_start_shutdown("finish");
      auto finished_complete = queue_.async_finish(
          *watcher_stream_, "leader_election_participant/shutdown/finish",
          jb::etcd::use_future());
      JB_LOG(trace) << log_header("") << "  finish scheduled";
      // TODO() - this is a workaround, the finish() call does not seem
      // to terminate for me ...
      if (finished_complete.wait_for(std::chrono::milliseconds(200)) ==
          std::future_status::timeout) {
        JB_LOG(info) << log_header("") << "  timeout on Finish() call";
        async_op_done("on_finish() - forced");
      }
    }
    (void)set_state("shutdown()", state::shutdown);
  }

  /// Kick-off a campaign and call a functor when elected
  template <typename Functor>
  void campaign(Functor&& callback) {
    campaign_impl(std::function<void(bool)>(std::move(callback)));
  }

  /**
   * Refactor template code via std::function
   *
   * The main "interface" is the campaing() template member functions,
   * but we loath duplicating that much code here, so refactor with a
   * std::function<>.  The cost of such functions is higher, but
   * leader election is not a fast operation.
   */
  void campaign_impl(std::function<void(bool)>&& callback) {
    JB_LOG(trace) << log_header("") << "  kicking off campaign";
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
    JB_LOG(trace) << log_header("") << " publish_value()";
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
      bool ok) {
    if (not ok) {
      make_callback(false);
    }
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
      JB_LOG(trace) << log_header("") << "  create watcher ... k=" << kv.key();
      watched_keys_.insert(kv.key());

      etcdserverpb::WatchRequest req;
      auto& create = *req.mutable_create_request();
      create.set_key(kv.key());
      create.set_start_revision(op.response.header().revision() - 1);

      queue_.async_write(
          *watcher_stream_, std::move(req),
          "leader_election_participant/on_range_request/watch",
          [ this, key = kv.key(),
            revision = op.response.header().revision() ](auto op, bool ok) {
            this->on_watch_create(op, ok, key, revision);
          });
    }
    check_election_over_maybe();
  }

  /// Called when a Write() operation that creates a watcher completes.
  void on_watch_create(
      watch_write_op const& op, bool ok, std::string const& wkey,
      std::uint64_t wrevision) {
    async_op_done("on_watch_create()");
    if (not ok) {
      JB_LOG(trace) << log_header("on_watch_create(.., false) wkey=") << wkey;
      return;
    }
    if (not async_op_start("read watch")) {
      return;
    }

    queue_.async_read(
        *watcher_stream_, "leader_election_participant/on_watch_create/read",
        [this, wkey, wrevision](auto op, bool ok) {
          this->on_watch_read(op, ok, wkey, wrevision);
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
      watch_read_op const& op, bool ok, std::string const& wkey,
      std::uint64_t wrevision) {
    async_op_done("on_watch_read()");
    if (not ok) {
      JB_LOG(trace) << log_header("on_watch_read(.., false) wkey=") << wkey;
      return;
    }
    if (op.response.created()) {
      JB_LOG(trace) << "  received new watcher=" << op.response.watch_id();
      std::lock_guard<std::mutex> lock(mu_);
      current_watches_.insert(op.response.watch_id());
    } else {
      JB_LOG(trace) << log_header("") << "  update for existing watcher="
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
      current_watches_.erase(op.response.watch_id());
      return;
    }
    if (op.response.compact_revision()) {
      // TODO() - if I am reading the documentation correctly, this
      // means the watcher was cancelled, but the data may (or may
      // not) still be there.  We need to worry about the case where
      // the participant figures out the key to watch.  Then it goes
      // to sleep, or gets rescheduled, then the key is deleted
      // and etcd compacted.  And then the client starts watching.
      //
      // I am not sure this is a problem, but it might be.
      //
      JB_LOG(info) << log_header("")
                   << " watcher cancelled with compact_revision="
                   << op.response.compact_revision() << ", wkey=" << wkey
                   << ", revision=" << wrevision
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
        *watcher_stream_, "leader_election_participant/on_watch_read/read",
        [this, wkey, wrevision](auto op, bool ok) {
          this->on_watch_read(op, ok, wkey, wrevision);
        });
  }

  /// Check if the election has finished, if so invoke the callbacks.
  void check_election_over_maybe() {
    // ... check the atomic, do not worry about changes without a lock,
    // if it is positive then a future Read() will decrement it and we
    // will check again ...
    {
      std::lock_guard<std::mutex> lock(mu_);
      if (not watched_keys_.empty()) {
        return;
      }
      if (state_ != state::shuttingdown and state_ != state::shutdown) {
        state_ = state::elected;
      }
    }
    JB_LOG(trace) << log_header("") << " election completed";
    make_callback(true);
  }

  // Invoke the callback, notice that the callback is invoked only once.
  void make_callback(bool result) {
    std::function<void(bool)> callback;
    {
      std::lock_guard<std::mutex> lock(mu_);
      callback = std::move(campaign_callback_);
    }
    if (not callback) {
      JB_LOG(trace) << log_header("") << " no callback present";
      return;
    }
    callback(result);
    JB_LOG(trace) << log_header("") << "  made callback";
  }

private:
  completion_queue_type& queue_;

  std::set<std::uint64_t> current_watches_;
  std::set<std::string> watched_keys_;

  std::function<void(bool)> campaign_callback_;
};

} // namespace detail
} // namespace etcd
} // namespace jb

#endif // jb_etcd_detail_leader_election_runner_impl_hpp
