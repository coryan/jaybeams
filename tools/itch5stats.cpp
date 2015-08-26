#include <jb/itch5/process_iostream.hpp>
#include <jb/config_object.hpp>
#include <jb/event_rate_histogram.hpp>
#include <jb/fileio.hpp>
#include <jb/histogram.hpp>
#include <jb/integer_range_binning.hpp>
#include <jb/log.hpp>

#include <iostream>
#include <stdexcept>

namespace {
class config : public jb::config_object {
 public:
  config();
  config_object_constructors(config);

  void validate() const override;

  jb::config_attribute<config,std::string> input_file;
};



std::size_t const aggregate_max_messages_per_sec = 1000000;
std::size_t const max_messages_per_sec  = 10000;
std::size_t const max_messages_per_msec = 10000;
std::size_t const max_messages_per_usec = 1000;
std::chrono::minutes report_period(60);

typedef std::chrono::high_resolution_clock clock;
typedef clock::duration duration;
typedef clock::duration::rep elapsed_histogram_type;
typedef jb::histogram<jb::integer_range_binning<elapsed_histogram_type>, unsigned long> elapsed_time_histogram;

/**
 * Collect the key message rate statistics.
 */
class message_rate_stats {
 public:
  message_rate_stats(int max_per_sec = max_messages_per_sec)
      : per_sec_rate_(max_per_sec, std::chrono::seconds(1))
      , per_msec_rate_(max_messages_per_msec, std::chrono::milliseconds(1))
      , per_usec_rate_(max_messages_per_usec, std::chrono::microseconds(1))
  {}

  void csv_stats(std::ostream& os) const {
    csv_rate(os, per_sec_rate_);
    csv_rate(os, per_msec_rate_);
    csv_rate(os, per_usec_rate_);
  }

  void report_stats(jb::timestamp ts) const {
    report_rate(ts, "sec ", per_sec_rate_);
    report_rate(ts, "msec", per_msec_rate_);
    report_rate(ts, "usec", per_usec_rate_);
  }

  void sample(jb::timestamp ts) {
    per_sec_rate_.sample(
        std::chrono::duration_cast<std::chrono::milliseconds>(ts));
    per_msec_rate_.sample(ts);
    per_usec_rate_.sample(ts);
  }

 private:
  template<typename event_rate>
  void report_rate(
      jb::timestamp ts, char const* period_name,
      event_rate const& histo) const {
    JB_LOG(info) << "events/" << period_name << ": " << jb::as_hhmmss(ts)
                 << "  min=" << histo.observed_min()
                 << ", max=" << histo.observed_max()
                 << ", mean=" << histo.estimated_mean()
                 << ", p50=" << histo.estimated_quantile(0.5)
                 << ", p99=" << histo.estimated_quantile(0.99)
                 << ", p99.9=" << histo.estimated_quantile(0.999)
                 << ", p99.99=" << histo.estimated_quantile(0.9999);
  }

  template<typename event_rate>
  void csv_rate(std::ostream& os, event_rate& histo) const {
    os << "," << histo.nsamples();
    if (histo.nsamples() == 0) {
      os << ",,,,,,";
      return;
    }
    os << "," << histo.estimated_mean()
       << "," << histo.observed_min()
       << "," << histo.estimated_quantile(0.5)
       << "," << histo.estimated_quantile(0.99)
       << "," << histo.estimated_quantile(0.999)
       << "," << histo.observed_max();
  }

 private:
  jb::event_rate_histogram<int,std::chrono::milliseconds> per_sec_rate_;
  jb::event_rate_histogram<> per_msec_rate_;
  jb::event_rate_histogram<> per_usec_rate_;
};

class itch5_stats_handler {
 public:
  itch5_stats_handler()
      : last_report_ts_()
      , aggregate_rate_stats_(aggregate_max_messages_per_sec)
      , processing_delay_(elapsed_time_histogram::binning_strategy(0, 1000000))
  {}

  template<typename message_type>
  void handle(long msgcnt, std::size_t msgoffset, message_type const& msg) {
    JB_LOG(trace) << msgcnt << ":" << msgoffset << " " << msg;
    jb::timestamp ts = std::chrono::duration_cast<std::chrono::microseconds>(
        msg.header.timestamp.ts);
    if (last_report_ts_ == jb::timestamp()) {
      last_report_ts_ = ts;
    }
    if (ts - last_report_ts_ >= report_period) {
      aggregate_rate_stats_.report_stats(ts);
      report_delay(ts, "delay      ", processing_delay_);
      last_report_ts_ = ts;
    }
    aggregate_rate_stats_.sample(ts);
  }

  void handle_unknown(
      char const* msgbuf, std::size_t msglen, long msgcnt,
      std::size_t msgoffset) {
    JB_LOG(error) << "Unknown message type '" << msgbuf[0] << "'"
                  << " in msgcnt=" << msgcnt << ", msgoffset=" << msgoffset;
  }

  static clock::time_point now() {
    return clock::now();
  }
        
  void handle_elapsed(clock::time_point start, clock::time_point end) {
    processing_delay_.sample((end - start).count());
  }

 private:
  void report_delay(
      jb::timestamp ts, char const* name,
      elapsed_time_histogram const& histo) const {
    if (histo.nsamples() == 0) {
      JB_LOG(info) << name << ": " << jb::as_hhmmss(ts)
                   << "  no data available";
      return;
    }
    JB_LOG(info) << name << ": " << jb::as_hhmmss(ts)
                 << "  min=" << histo.observed_min()
                 << ", max=" << histo.observed_max()
                 << ", mean=" << histo.estimated_mean()
                 << ", p50=" << histo.estimated_quantile(0.5)
                 << ", p99=" << histo.estimated_quantile(0.99)
                 << ", p99.9=" << histo.estimated_quantile(0.999)
                 << ", p99.99=" << histo.estimated_quantile(0.9999);
  }
  
 private:
  jb::timestamp last_report_ts_;
  message_rate_stats aggregate_rate_stats_;
  elapsed_time_histogram processing_delay_;
};

} // anonymous namespace

int main(int argc, char* argv[]) try {
  config cfg;
  cfg.load_overrides(
      argc, argv, std::string("itch5_stats.yaml"), "JB_ROOT");
  jb::log::init();

  boost::iostreams::filtering_istream in;
  jb::open_input_file(in, cfg.input_file());

  itch5_stats_handler handler;
  process_itch5_stream(in, handler);

  return 0;
} catch(jb::usage const& u) {
  std::cerr << u.what() << std::endl;
  return u.exit_status();
} catch(std::exception const& ex) {
  std::cerr << "Standard exception raised: " << ex.what() << std::endl;
  return 1;
} catch(...) {
  std::cerr << "Unknown exception raised" << std::endl;
  return 1;
}

namespace {
config::config()
    : input_file(desc("input-file").help(
        "An input file with ITCH-5.0 messages."), this)
{}

void config::validate() const {
  if (input_file() == "") {
    throw jb::usage(
        "Missing input-file setting."
        "  You must specify an input file.", 1);
  }
}
} // anonymous namespace
