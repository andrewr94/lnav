
include $(top_srcdir)/aminclude_static.am

TESTS_ENVIRONMENT = $(SHELL) $(top_builddir)/TESTS_ENVIRONMENT
LOG_COMPILER = $(SHELL) $(top_builddir)/TESTS_ENVIRONMENT

BUILT_SOURCES = tailerbin.h tailerbin.cc

AM_CPPFLAGS = \
	-Wall \
	$(CODE_COVERAGE_CPPFLAGS) \
	$(LIBARCHIVE_CFLAGS) \
	$(READLINE_CFLAGS) \
	$(SQLITE3_CFLAGS) \
	$(LIBCURL_CPPFLAGS)

AM_LIBS = $(CODE_COVERAGE_LIBS)
AM_CFLAGS = $(CODE_COVERAGE_CFLAGS)
AM_CXXFLAGS = $(CODE_COVERAGE_CXXFLAGS)

dist_noinst_DATA = \
    tailer.ape

noinst_LIBRARIES = \
    libtailercommon.a \
    libtailerpp.a \
    libtailerservice.a

noinst_HEADERS = \
    sha-256.h \
    tailer.h \
    tailer.looper.hh \
    tailer.looper.cfg.hh \
    tailerpp.hh

libtailercommon_a_SOURCES = \
    sha-256.c \
    tailer.c

libtailerpp_a_CPPFLAGS = \
    $(AM_CPPFLAGS) \
    -I$(srcdir)/.. \
    -I$(srcdir)/../fmtlib

libtailerpp_a_SOURCES = \
    tailerpp.cc

tailerbin.h tailerbin.cc: tailer tailer.ape ../tools/bin2c$(BUILD_EXEEXT)
	../tools/bin2c$(BUILD_EXEEXT) -n tailer_bin tailerbin $(srcdir)/tailer.ape

libtailerservice_a_CPPFLAGS = \
    $(AM_CPPFLAGS) \
    -I$(srcdir)/.. \
    -I$(srcdir)/../fmtlib

libtailerservice_a_LIBADD = \
    libtailercommon.a \
    libtailerpp.a

libtailerservice_a_SOURCES = \
    tailerbin.cc \
    tailer.looper.cc

check_PROGRAMS = \
    drive_tailer \
    tailer

tailer_SOURCES = \
    tailer.main.c

tailer_LDADD = libtailercommon.a

drive_tailer_CPPFLAGS = \
    -I$(srcdir)/.. \
    -I$(srcdir)/../fmtlib

drive_tailer_SOURCES = \
    drive_tailer.cc

drive_tailer_LDADD = \
    libtailercommon.a \
    libtailerpp.a \
    ../base/libbase.a \
    ../fmtlib/libcppfmt.a

dist_noinst_SCRIPTS = \
    test_tailer.sh

TESTS = \
    test_tailer.sh

DISTCLEANFILES = \
	*.dat \
	*.err \
	*.db \
	*.dpt \
	*.diff \
	*.index \
	*.tmp \
	*.errbak \
	*.tmpbak \
    tailerbin.h \
    tailerbin.cc

distclean-local:
	$(RM_V)rm -f foo
