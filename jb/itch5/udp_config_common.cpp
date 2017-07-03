#include "jb/itch5/udp_config_common.hpp"

namespace jb {
namespace itch5 {

udp_config_common::udp_config_common()
    : debug(
          desc("debug").help("Set the SO_DEBUG option on the socket.  On "
                             "Linux, if the user has the right capabilities, "
                             "this will enable debug messages to syslogd(8)."),
          this, false)
    , do_not_route(
          desc("do-not-route")
              .help("Set the SO_DONTROUTE option on the socket."),
          this, false)
    , linger(
          desc("linger").help("Set the SO_LINGER option on the socket."
                              "  This makes the socket block on shutdown(2) or "
                              "close(2) operations until all queued messages "
                              "have been sent, or to timeout after the value "
                              "set in --linger-seconds."),
          this, false)
    , linger_seconds(
          desc("linger-seconds")
              .help("Set the timeout value for the SO_LINGER "
                    "option.  The default is 0."),
          this, 0)
    , reuse_address(
          desc("reuse-address")
              .help("Set the SO_REUSEADDR option on the socket."
                    "  This makes it possible to bind multiple UDP sockets to "
                    "the same port, for example to receive multicast messages "
                    "in multiple applications."),
          this, true)
    , receive_buffer_size(
          desc("receive-buffer-size")
              .help("Set the SO_RCVBUF option -- the maximum socket receive "
                    "buffer.  By default, or if set to -1, use the system "
                    "defaults."),
          this, -1)
    , receive_low_watermark(
          desc("receive-low-watermark")
              .help("Set the SO_RCVLOWAT option -- the minimum number of bytes "
                    "in the receive buffer before the application is supposed "
                    "to be notified.  By default, or if set to -1, use the "
                    "system defaults."
                    "  On Linux consult socket(7) for caveats."),
          this, -1)
    , send_buffer_size(
          desc("send-buffer-size")
              .help("Set the SO_SNDBUF option -- the maximum socket send "
                    "buffer.  By default, or if set to -1, use the system "
                    "defaults."),
          this, -1)
    , send_low_watermark(
          desc("send-low-watermark")
              .help("Set the SO_SNDLOWAT option -- the minimum number of bytes "
                    "in the send buffer before the socket passes on the data to"
                    " protocol.  By default, or if set to -1, use the "
                    "system defaults."
                    "  On Linux consult socket(7) for caveats."),
          this, -1) {
}

} // namespace itch5
} // namespace jb
