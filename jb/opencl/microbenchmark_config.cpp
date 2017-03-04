#include "jb/opencl/microbenchmark_config.hpp"

jb::opencl::microbenchmark_config::microbenchmark_config()
    : microbenchmark(desc("microbenchmark", "microbenchmark"), this)
    , log(desc("log", "log"), this)
    , opencl(desc("opencl"), this) {
}
