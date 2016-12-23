#ifndef jb_detail_os_error_hpp
#define jb_detail_os_error_hpp

namespace jb {
namespace detail {

/**
 * Check the value of result and raise an exception if it is -1.
 */
void os_check_error(int result, char const* msg);

} // namespace detail
} // namespace jb

#endif // jb_detail_os_error_hpp
