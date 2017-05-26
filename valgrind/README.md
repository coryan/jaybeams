## Valgrind Headers

[valgrind](http://valgrind.org/) is a memory checking tool for C/C++.
The JayBeams test fail under valgrind because they use the `long
double` type, which
is
[not supported](http://valgrind.10908.n7.nabble.com/valgrind-does-not-handle-long-double-td41633.html) by
valgrind.
Generally this is not a problem,
running the tests without valgrind has established the results to be
functionally correct (or accurate enough if you prefer),
we run the tests under valgrind to check for memory management errors.
Because the code in JayBeams is parametric on the floating point
precision -- the same code works for `float`, `double`, and `long
double` -- we can assume that if valgrind does not detect memory
management errors for `float` nor `double` it is unlikely that there
are memory management errors for `long double`.
Therefore the tests for `long double` are disabled when executed under
valgrind.

The [recommended](http://valgrind.org/docs/manual/manual-core-adv.html#manual-core-adv.clientreq) way to detect if a test is running under valgrind is
to include `valgrind/valgrind.h` and call the `RUNNING_ON_VALGRIND`
macro, and to distribute the headers with the code, which we do here.
