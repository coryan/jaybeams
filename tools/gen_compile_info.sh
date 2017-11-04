#!/bin/sh

CXX=$1
CXXFLAGS=$*

echo '#include <jb/testing/compile_info.hpp>'
echo ''
echo 'char const jb::testing::uname_a[] = R"""('
uname -a
echo ')""";'
echo ''
echo 'char const jb::testing::compiler[] = R"""('
${CXX?} --version
echo ')""";'
echo ''
echo 'char const jb::testing::compiler_flags[] = R"""('
echo ${CXXFLAGS}
echo ')""";'
echo ''
echo 'char const jb::testing::linker[] = R"""('
${CXX?} -Wl,--version 2>/dev/null
echo ')""";'
echo ''
echo 'char const jb::testing::gitrev[] = R"""('
git rev-parse HEAD 2>/dev/null
echo ')""";'
echo ''

exit 0
