# JayBeams uses a non-recursive make, this is the only Makefile.

ACLOCAL_AMFLAGS = -I m4
instdir = $(prefix)/lib
inst_LIBRARIES = \
  jb/libjb.a

@CODE_COVERAGE_RULES@
AM_CXXFLAGS = $(CODE_COVERAGE_CFLAGS)
LIBS += $(CODE_COVERAGE_LDFLAGS)

################################################################
# jb/libjb.a
################################################################

jb_libjb_adir = $(includedir)
nobase_jb_libjb_a_HEADERS =
jb_libjb_a_SOURCES =
jb_libjb_a_LIBADD = 
