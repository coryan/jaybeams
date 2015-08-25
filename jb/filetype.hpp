#ifndef jb_filetype_hpp
#define jb_filetype_hpp

#include <string>

namespace jb {

/**
 * Return true if the filename ends in .gz
 */
bool is_gz(std::string const& filename);

} // namespace jb

#endif // jb_filetype_hpp
