#ifndef jb_testing_future_status_hpp
#define jb_testing_future_status_hpp
/**
 * @file
 *
 * Helper function for Boost.Tests that use std::future_status.
 */
#include <future>
#include <iostream>

namespace std {
// Introduce a streaming operator to satify Boost.Test needs.
inline std::ostream& operator<<(std::ostream& os, std::future_status x) {
  if (x == std::future_status::timeout) {
    return os << "[timeout]";
  }
  if (x == std::future_status::ready) {
    return os << "[ready]";
  }
  return os << "[deferred]";
}
}

#endif // jb_testing_future_status_hpp
