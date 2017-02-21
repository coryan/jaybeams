# JayBeams Style Guide and Coding Standards

## Introduction

This document describes the coding standards used in JayBeams, and the
rationale behind them.  If you disagree with the standard please open
a issue in github and/or send a PR to change this document before
changing the code.

## Header Files

### All headers shoudl be self-contained.

One should be able to `#``include` a header and compile it without
requiring any additional headers before or after.

### The `#`define Guard

All headers should include a `#``ifndef/``#``define/``#``endif` guard.
The guard must be the first line in the header.
The name of the guard is constructed by the full path of the header
file from the root of JayBeams, replacing both slashes (`/`) and
period (`.`) characters by an underscore (`_`).
For example, the include guard for `foo/bar/baz_qux.hpp` becomes
`foo_bar_baz_qux_hpp`.
There must be no double underscores in a guard name, that limits your
options for filenames.

**Example:** the `foo/bar/baz_qux.hpp` file **must** be protected by:

```c++
#ifndef foo_bar_baz_qux_hpp
#define foo_bar_baz_qux_hpp
// ... your code here
#endif // foo_bar_baz_qux_hpp
```

**Rationale:** protecting against multiple includes is a standard C++
  practice, we will not elaborate further.  Making that the first line
  enables some compilers to optimize parsing of the header if it is
  already included.  Making the name of the guard algorithmic removes
  a debate.  Identifiers with double underscores are reserved for the
  implementation in POSIX, so we avoid them.

### Forward Declarations

Use them when needed or they make sense.

### Inline Functions

Single line functions should be inlined, virtual functions, even if
single line, should not be inlined.
Inline function should not be separate from the class definition.

**Use:**

```c++
class Foo {
public:
  inline int answer() const {
    return 42;
  }
```

**Do Not Use:**

```c++
class Foo {
public:
  int answer() const;
};
inline int Foo::answer() const {
  return 42;
}
```

**Rationale:** We want to avoid function call overhead for trivial
  functions, but inlining rarely works for virtual functions.
  We want to minimize typing.

### Name of Includes

The name of the include must match the name of the main class or
function in the include.
Define a single class per include, do not define many classes in the
same header.
Multiple function overloads can be included in the same header, but
avoid many unrelated functions in the same header.

### Order of Includes

The main include for a source file goes first.
The includes in the same directory,
then includes in JayBeams,
then boost and other third-party libraries,
then C++ libraries.

The order is enforced by clang-format, so we do not define it further,
the tool does it for us.

**Rationale:** Lakos' argument, we want to include the header early to
detect missing dependencies as soon as possible.

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

## Aliases

Use aliases to increase clarity, specially in template code where type
traits are a common practice to improve readability and usability.
Prefer the C++11 `using foo = bar;` aliases over the `typedef bar
foo;` aliases.

**Rationale:** The use of type aliases is a standard practice, we will
  not elaborate further.
  The use of `using` aliases vs. `typedef` aliases is for
  consistency: `using` is required for template aliases, so we use it
  everywhere.

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
