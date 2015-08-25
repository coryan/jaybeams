#ifndef jb_fileio_hpp
#define jb_fileio_hpp

#include <boost/iostreams/filtering_stream.hpp>

#include <string>

namespace jb {

/**
 * Open a file for writing.
 */
void open_output_file(
    boost::iostreams::filtering_ostream& out,
    std::string const& filename);

/**
 * Open a file for reading.
 */
void open_input_file(
    boost::iostreams::filtering_istream& in,
    std::string const& filename);

} // namespace jb

#endif // jb_fileio_hpp
