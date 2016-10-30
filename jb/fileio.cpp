#include <jb/fileio.hpp>
#include <jb/filetype.hpp>

#include <boost/iostreams/filter/gzip.hpp>
#include <boost/iostreams/device/file.hpp>
#include <boost/iostreams/filtering_stream.hpp>

#include <fstream>
#include <iostream>

void jb::open_output_file(boost::iostreams::filtering_ostream& out,
                          std::string const& filename) {

  if (filename == "stdout") {
    out.push(std::cout);
    return;
  }
  if (jb::is_gz(filename)) {
    out.push(boost::iostreams::gzip_compressor());
  }
  boost::iostreams::file_sink file(filename,
                                   std::ios_base::out | std::ios_base::binary);
  out.push(file);
}

void jb::open_input_file(boost::iostreams::filtering_istream& in,
                         std::string const& filename) {

  boost::iostreams::file_source file(filename,
                                     std::ios_base::in | std::ios_base::binary);
  if (jb::is_gz(filename)) {
    in.push(boost::iostreams::gzip_decompressor());
  }
  in.push(file);
}
