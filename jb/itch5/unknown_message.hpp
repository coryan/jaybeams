#ifndef jb_itch5_unknown_message_hpp
#define jb_itch5_unknown_message_hpp

#include <jb/itch5/message_header.hpp>

namespace jb {
namespace itch5 {

/**
 * A simple 
 */
class unknown_message {
 public:
  /// Constructor from message details
  unknown_message(
      std::uint32_t count,
      std::size_t offset,
      std::size_t len,
      void const* buf)
      : count_(count)
      , offset_(offset)
      , len_(len)
      , buf_(buf)
  {}
  unknown_message(unknown_message&&) = default;

  //@{
  /**
   * @name Accessors
   */
  std::uint32_t count() const {
    return count_;
  }
  std::uint64_t offset() const {
    return offset_;
  }
  std::size_t len() const {
    return len_;
  }
  void const* buf() const {
    return buf_;
  }
  //@}

  /// Extract the message header
  template<bool validate>
  message_header decode_header() const {
    return decoder<validate,message_header>::r(
        len(), static_cast<char const*>(buf()), 0);
  }

 private:
  // Prohibit copy and assignment
  unknown_message(unknown_message const&) = delete;
  unknown_message& operator=(unknown_message const&) = delete;

 private:
  std::uint32_t count_;
  std::size_t offset_;
  std::size_t len_;
  void const* buf_;
};

} // namespace itch5
} // namespace jb

#endif // jb_itch5_unknown_message_hpp
