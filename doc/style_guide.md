# JayBeams Style Guide and Coding Standards

## Introduction

This document describes the coding standards used in JayBeams, and the
rationale behind them.  If you disagree with the standard please open
a issue in github and/or send a PR to change this document before
changing the code.

## Naming Guidelines

### Identifiers

Identifiers use lowercase characters, with words separated by
underscore characters.  All global identifiers (except pre-processor
macros) must be in the `jb::` namespace.

**Rationale:** this is the naming adopted in the C++ standard and
  Boost.  It used to be the case that uppercase names were "safer" for
  the application, because in implementations lacking namespaces the
  system or standard could introduce a conflicting name without
  warning.

### Function-like Macros

In general JayBeams tries to not use macros, but they are sometimes
the best (or only) way to implement a feature.  Function-like macros
should be named like functions, using lowercase characters with words
separated by underscores.

### Object-like Macros

The only object-like macros used in JayBeams are intended to set
compile-time configurable parameters.
Such macros are used in a single .cpp file, start with the path of the
file in ALL CAPS, followed by the name of the constant in all lowercase.

For example, suppose a compile-time configurable parameter is called
`max_slots` and is used in `jb/foo_bar/baz.cpp`, the macro name shall
be `JB_FOO_BAR_BAZ_max_slots`.

### Namespaces

Namespaces are also lowercase, with the path of the namespace matching
the path of the files.  So, for example, all classes and functions in
the `jb::foo::bar` namespace must be defined in files contained by the
`jb/foo/bar` directory.

**Rationale:** we want to make it easy to find classes and functions.

#### Exception: the `jb::*::defaults` namespaces

JayBeams defines a series of namespaces containing default values for
configuration parameters.  The objects in these namespaces do not need
a sub-directory.

**Rationale:** the namespace simply defines a number of constants,
  typically used in a single `.cpp` file, creating a complete new
  directory for them is overkill.

### Filenames

Filenames are all lowercase, no file can be longer than 128 bytes in
length.
The file name should match the name of the main class or function
defined in the class.

**Rationale:** This makes it easier to find the classes and
  functions.  Always using lowercases avoid conflicts in operating
  systems that do not distinguish between uppercase and lowercase
  characters in filenames.  Some modern operating systems impose
  limits on filename length [ref](https://en.wikipedia.org/wiki/Comparison_of_file_systems)

### No "util" Files

**Rationale:** Util files become hard to maintain and distinguish from
each other, if you have standalone functions just create a separate
pair of files for them.

## Use Exceptions Sparingly

Exceptions should represent, well, exceptional conditions.  Normal
errors that we expect (e.g. missing files, closed sockets, etc) are
not exceptional conditions and the API should include treatment for
them.

**Rationale:** this is the standard practice in C++, read "The Design
  and Evolution of C++".

## External Depedendencies

JayBeams will prefer to use an external dependency over reimplementing
significant functionality.  Obviously there is a tradeoff for very
small external dependencies, developers should apply their best
judgement.  As a guideline, external libraries that implement less
than 100 lines of code are probably too uninteresting to depend on.

### Preferred Libraries

JayBeams will prefer to use functionality from the standard C++
library when available.  If not available, JayBeams will prefer to use
libraries for [boost](http://www.boost.org).  If neither the C++
standard nor Boost provide the desired functionality then the
developers should describe the dependency and document the rationale
for it in this document.

#### FFTW Library

Implement the Fast Fourier Transform (FFT) algorithm for modern CPUs,
taking advantage of vectorized instructions and many other
optimizations.

**Rationale:** Largely grandfathered in.  Has a good reputation as a
fast FFT library.

#### YAML-CPP Library

Implement a [YAML](http://www.yaml.org) parser for C++.

**Rationale:** Largely grandfathered in.

#### Boost.Compute Library

Boost wrappers for OpenCL.

**Rationale:** accepted into boost.org as of 1.61.0, though not
included in many Linux distributions yet.  Wanted better (safer, more
expressive) wrappers than what OpenCL provides natively.

#### clFFT Library

A FFT library for OpenCL implemented by AMD.

**Rationale:** Largely grandfathered in.

## Formatting and Whitespace

JayBeams uses
[clang-format](http://clang.llvm.org/docs/ClangFormat.html) for
low-level formatting issues.  We use version 3.8 or higher to do the
formatting.  The configuration file is included with
the source code.

Notice that the formatting prescribes the order of `#``include`
directives.
We follow the suggestions from Lakos: first include the main header file,
then local files, the increasingly lower dependencies.  The objective
is to detect missing dependencies in the header files early.
With `clang-format` the main header file must be labeled specially by
using double quotes instead of angle brackets for the include
directive.  That is, please use `#``include "foo/bar/baz.h"` instead of
`#``include <foo/bar/baz.h>`.

**Rationale:** arguing about whitespace is not productive, let the
robots do that.
