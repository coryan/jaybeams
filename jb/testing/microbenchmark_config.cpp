#include "jb/testing/microbenchmark_config.hpp"

namespace jb {
namespace testing {
/**
 * Define the default values for a microbenchmark config.
 *
 * This is an experiment in how to write highly-configurable classes
 * for JayBeams.  The intention is that any configuration parameter
 * can be set using these rules:
 *
 * -# if the configuration is set in the command-line, use the
 *    command-line value, otherwise
 * -# if the configuration is set in the configuration file, use the
 *    value in the configuration file, otherwise
 * -# if the program was compiled with a special setting for the
 *    default value, use that setting, otherwise
 * -# use whatever default was chosen by the developer.
 *
 * The configuration files themselves are found using a series of
 * rules (TBD).
 *
 * What I would want is that each component can define its own config
 * class, and the main program *choses* which ones become command-line
 * arguments.
 */
namespace defaults {

#ifndef JB_DEFAULTS_microbenchmark_warmup_iterations
#define JB_DEFAULTS_microbenchmark_warmup_iterations 100
#endif

#ifndef JB_DEFAULTS_microbenchmark_iterations
#define JB_DEFAULTS_microbenchmark_iterations 1000
#endif

#ifndef JB_DEFAULTS_microbenchmark_size
#define JB_DEFAULTS_microbenchmark_size 0
#endif

#ifndef JB_DEFAULTS_microbenchmark_verbose
#define JB_DEFAULTS_microbenchmark_verbose false
#endif

int warmup_iterations = JB_DEFAULTS_microbenchmark_warmup_iterations;
int iterations = JB_DEFAULTS_microbenchmark_iterations;
int size = JB_DEFAULTS_microbenchmark_size;
bool verbose = JB_DEFAULTS_microbenchmark_verbose;

} // namespace defaults

microbenchmark_config::microbenchmark_config()
    : warmup_iterations(
          desc("warmup-iterations")
              .help("The number of warmup iterations in the benchmark."),
          this, defaults::warmup_iterations)
    , iterations(desc("iterations")
                     .help("Number of iterations to run for a fixed size."),
                 this, defaults::iterations)
    , size(desc("size")
               .help("If set (and not zero) control the size of the test."),
           this, defaults::size)
    , verbose(desc("verbose")
                  .help("If true, dump the results of every test to stdout for"
                        " statistical analysis."),
              this, defaults::verbose)
    , test_case(desc("test-case")
                    .help("Some microbenchmarks test completely different "
                          "configurations"
                          ", settings, or even different algorithms for the "
                          "same problem."
                          "  Use this option to configure such benchmarks"
                          ", most microbenchmarks will ignore it."),
                this) {
}

} // namespace testing
} // namespace jb
