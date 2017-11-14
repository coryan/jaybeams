# JayBeams Tools

The JayBeams tools serve both as (a) demonstrations of how to use the
library in complete programs, and (b) utilities to analyze market data
feeds to help in the design of the libraries and/or in writing notes
about them.

These options for each program can be found by executing each program
with the --help option, for example, after you compile the tools you
could use:

    cd tools
    ./itch5inside --help
    Program Options:
      --help  ....

All the program options can also be specified in a
[YAML](http://www.yaml.org/) file.
If you 
find yourself typing the same options over and over you might consider
creating a configuration file instead.  You can always override the
configurations from the file with command-line options.

For example, if you are running the `itch5inside` program with the
same input file and varying other options you could create a
`itch5inside.yaml` with the following contents:

    input-file: /foo/bar/baz/02022015.NASDAQ_ITCH50.gz
    enable-symbol-stats: 1
    stats:
      max-messages-per-microsecond: 100

You can read more about how JayBeams setup configuration files in the
[documentation](doc/jb_configuration.html)
