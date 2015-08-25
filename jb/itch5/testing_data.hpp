#ifndef jb_itch5_testing_data_hpp
#define jb_itch5_testing_data_hpp

/// A convenient sequence of bytes to test messages.
#define JB_ITCH5_TEST_HEADER \
  "\x00\x00"                 /* Stock Locate    (0) */ \
  "\x00\x01"                 /* Tracking Number (1) */ \
  "\x25\xCA\x5F\xF4\x23\x15" /* Timestamp       (11:32:30.123456789) */

#endif /* jb_itch5_testing_data_hpp */
