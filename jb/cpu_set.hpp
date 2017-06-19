#ifndef jb_cpu_set_hpp
#define jb_cpu_set_hpp

#include <iosfwd>
#include <sched.h>
#include <string>

namespace jb {

/**
 * A wrapper for the Linux CPU_SET data structure.
 *
 * Setting CPU affinity can help improve the predictability of our
 * system.  Typically different threads are assigned to different
 * processors and in extreme cases no threads share the same core.
 * The cpu sets are also typically configured using the "List Format"
 * as described in cpuset(7).  Briefly this format is:
 *
 * cpuset ::= range[,cpuset]
 * range  ::= (number|number-number)
 * number ::= a positive integer
 *
 * So one can specify a cpu_set as "1,2,3" (cpus 1, 2 and 3), or
 * "1-15" (cpus from 1 to 15), or "0-4,10,12-14" (cpus 0 to 4, 10 and
 * 12 to 14).
 *
 * The iostream operators (used for tests, and conversions everywhere)
 * use this format.
 *
 * TODO(#151) this is a portability problem.  Many platforms can
 * specify cpu affinity, but they all use different constructs for
 * it.
 * TODO(#152) we only support up to CPU_SETSIZE cpus, it is possible
 * to control more CPUs in Linux, but the application that has access
 * to more than 1024 cpus is rare indeed.
 */
class cpu_set {
public:
  cpu_set()
      : set_() {
    CPU_ZERO(&set_);
  }

  /// Return the number of CPUs that can be stored in the cpu set.
  std::size_t capacity() const {
    return CPU_SETSIZE;
  }

  /// Returns true if @a cpu is included in the cpu set.
  bool status(int cpu) const {
    return CPU_ISSET(cpu, &set_);
  }

  /// Return the number of cpus included in the cpu set.
  int count() const {
    return CPU_COUNT(&set_);
  }

  /// Remove all cpus from the cpu set.
  cpu_set& reset() {
    CPU_ZERO(&set_);
    return *this;
  }

  /// Add @a cpu to the cpu set.
  cpu_set& set(int cpu) {
    check_range(cpu, "set");
    CPU_SET(cpu, &set_);
    return *this;
  }

  /// Remove @a cpu from the cpu set.
  cpu_set& clear(int cpu) {
    check_range(cpu, "clear");
    CPU_CLR(cpu, &set_);
    return *this;
  }

  /// Add all the cpus in the [cpulo,cpuhi] range to the set.
  cpu_set& set(int cpulo, int cpuhi);

  /// Remove all the cpus in the [cpulo,cpuhi] range from the set.
  cpu_set& clear(int cpulo, int cpuhi);

  //@{
  /**
   * @name Implement bitwise operations for cpu sets.
   */
  void operator&=(cpu_set const& rhs) {
    (void)CPU_AND(&set_, &set_, &rhs.set_);
  }
  void operator|=(cpu_set const& rhs) {
    (void)CPU_OR(&set_, &set_, &rhs.set_);
  }
  void operator^=(cpu_set const& rhs) {
    (void)CPU_XOR(&set_, &set_, &rhs.set_);
  }
  //@}

  //@{
  /**
   * @name Comparison operators for cpu sets.
   */
  bool operator==(cpu_set const& rhs) const {
    return CPU_EQUAL(&set_, &rhs.set_);
  }

  bool operator!=(cpu_set const& rhs) const {
    return not(*this == rhs);
  }
  //@}

  /**
   * Interpret @a value as a cpu set in list format.
   */
  static cpu_set parse(std::string const& value);

  /**
   * Return the set in the list format representation.
   */
  std::string as_list_format() const;

  //@{
  /**
   * @name Access the native implementation.
   */
  cpu_set_t* native_handle() {
    return &set_;
  }
  cpu_set_t const* native_handle() const {
    return &set_;
  }
  //@}

private:
  /**
   * Check that @a cpu is in range
   *
   * @throw std::out_of_range if the argument is out of range.
   */
  void check_range(int cpu, char const* op) const;

  /**
   * Check that @a [cpulo,cpuhi] is in range
   *
   * @throw std::out_of_range if the arguments are out of range.
   */
  void check_range(int cpulo, int cpuhi, char const* op) const;

  /**
   * Raise an exception because the input to parse() is invalid.
   */
  static void parse_error(std::string const& value);

private:
  cpu_set_t set_;
};

/// Stream a cpu set in list format.
std::ostream& operator<<(std::ostream&, cpu_set const&);

/// Read a cpu set in list format.
std::istream& operator>>(std::istream&, cpu_set&);

/// Bitwise AND operator for cpu sets
inline cpu_set operator&(cpu_set const& lhs, cpu_set const& rhs) {
  cpu_set r = lhs;
  r &= rhs;
  return r;
}

/// Bitwise OR operator for cpu sets
inline cpu_set operator|(cpu_set const& lhs, cpu_set const& rhs) {
  cpu_set r = lhs;
  r |= rhs;
  return r;
}

/// Bitwise XOR operator for cpu sets
inline cpu_set operator^(cpu_set const& lhs, cpu_set const& rhs) {
  cpu_set r = lhs;
  r ^= rhs;
  return r;
}

//@}

} // namespace jb

#endif // jb_cpu_set_hpp
