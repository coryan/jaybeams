#include "jb/security_directory.hpp"
#include <jb/security.hpp>

#include <sstream>
#include <stdexcept>

namespace jb {

std::shared_ptr<security_directory> security_directory::create_directory() {
  return std::shared_ptr<security_directory>(new security_directory);
}

security_directory::security_directory()
    : mutex_()
    , generation_(0)
    , contents_()
    , reverse_index_() {
}

security security_directory::insert(std::string symbol) {
  // Inserts are slow operations that always require a mutex ...
  std::lock_guard<std::mutex> guard(mutex_);
  // ... find or create the entry ...
  std::shared_ptr<security_directory_entry> entry =
      insert_unlocked(std::move(symbol));
  // ... return a new security with the contents ...
  return security(shared_from_this(), entry->id, generation_.load(), entry);
}

security security_directory::insert(std::string const& symbol, int id_hint) {
  // Inserts are slow operations that always require a mutex ...
  std::lock_guard<std::mutex> guard(mutex_);
  // let's find out if the id_hint was useful ...
  std::shared_ptr<security_directory_entry> entry;
  if (id_hint < 0 or id_hint > contents_.size()) {
    // ... out of range hint, just ignore it ...
    entry = insert_unlocked(symbol);
  } else if (contents_[id_hint]->symbol != symbol) {
    // ... mismatched hint is ignored too ...
    entry = insert_unlocked(symbol);
  } else {
    // ... okay, the hint was useful, just return a security for that
    // entry ...
    entry = contents_[id_hint];
  }
  return security(shared_from_this(), entry->id, generation_.load(), entry);
}

std::shared_ptr<security_directory_entry>
security_directory::insert_unlocked(std::string symbol) {
  // In all this code we assume the lock is held.  First lookup the
  // security by its symbol ...
  auto loc = reverse_index_.find(symbol);
  if (loc != reverse_index_.end()) {
    // ... if we found it, simply return the existing entry ...
    return contents_[loc->second];
  }
  // ... it was not there, we need to create the new entry ...
  auto attributes = std::make_shared<security_directory_attributes>();
  auto entry = std::shared_ptr<security_directory_entry>(
      new security_directory_entry{static_cast<int>(contents_.size()),
                                   std::move(symbol), attributes});
  // ... and modify the index by id and by symbol ...
  contents_.emplace_back(entry);
  reverse_index_[entry->symbol] = entry->id;
  // ... that requires an update to the generation counter, which can
  // use relaxed memory ordering because the lock will take care of
  // the cache invalidation ...
  generation_.fetch_add(1, std::memory_order_relaxed);
  return entry;
}

std::shared_ptr<security_directory_entry>
security_directory::check_and_refresh_cached_attributes(
    security const& sec) const {
  // If the (atomic) generation counter has not changed since the last
  // time then the previously cached entry is still valid, just return
  // that ...
  // TODO(...) we could explore more relaxed memory models here
  if (sec.generation_.load() == generation_.load()) {
    // ... potentially the member variable "generation" is changing
    // and we are missing updates.  This is unavoidable even with full
    // locking, we accept it as the cost of using lock-free algorithms
    // ...
    return sec.entry_;
  }
  // ... needs an update, this is the slow case that needs full
  // locking ...
  std::lock_guard<std::mutex> guard(mutex_);
  if (sec.id_ < 0 or sec.id_ > contents_.size()) {
    std::ostringstream os;
    os << "invalid security id " << sec.id_ << ", expected range was [0,"
       << contents_.size() << ")";
    throw std::runtime_error(os.str());
  }
  // ... copy the new entry into the security ...
  sec.entry_ = contents_[sec.id_];
  // ... update the generation counter, this can be done with relaxed
  // memory order because the mutex provides all the synchronization
  // we need ...
  sec.generation_.store(
      generation_.load(std::memory_order_relaxed), std::memory_order_relaxed);
  return sec.entry_;
}

int security_directory::security_id(security const& sec) const {
  return sec.id_;
}

} // namespace jb
