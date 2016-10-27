#ifndef jb_itch5_compute_book_depth_hpp
#define jb_itch5_compute_book_depth_hpp

#include <jb/itch5/compute_base.hpp>

namespace jb {
namespace itch5 {

/** 
 * An implementation derivated fomr compute_base to callback the book depth.
 */

class compute_book_depth : public compute_base {
 public:
  //@{
  /**
   * @name Type traits
   */
  /// Callbacks
  typedef std::function<void(time_point,
			     message_header const&,
			     stock_t const&,
			     book_depth_t const&)
			> callback_type;
  //@}

  /// Initialize an empty handler and its call condition
  compute_book_depth(callback_type const& _callback)
    : callback_(_callback) {};

  /**
   * Verify  callback condition, call the callback if successful.
   */
  void call_callback_condition(time_point,
			       message_header const&,
			       stock_t const&,
			       half_quote const&,
			       half_quote const&,
			       book_depth_t const,
			       bool const) override;
  
 private:
  /// Store the callback ...
  callback_type callback_;
};

} // namespace itch5
} // namespace jb

#endif // jb_itch5_compute_book_depth_hpp
