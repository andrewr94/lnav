
include $(top_srcdir)/aminclude_static.am

AM_CPPFLAGS = \
	$(CODE_COVERAGE_CPPFLAGS) \
    -Wall \
    -I$(top_srcdir)/src/ \
    -I$(top_srcdir)/src/third-party \
    -I$(top_srcdir)/src/fmtlib \
    $(LIBARCHIVE_CFLAGS) \
    $(READLINE_CFLAGS) \
    $(SQLITE3_CFLAGS) \
    $(LIBCURL_CPPFLAGS)

AM_LIBS = $(CODE_COVERAGE_LIBS)
AM_CFLAGS = $(CODE_COVERAGE_CFLAGS)
AM_CXXFLAGS = $(CODE_COVERAGE_CXXFLAGS)

noinst_LIBRARIES = libbase.a

noinst_HEADERS = \
    auto_fd.hh \
    auto_pid.hh \
    date_time_scanner.hh \
    enum_util.hh \
    file_range.hh \
    fs_util.hh \
    func_util.hh \
    future_util.hh \
	humanize.hh \
	humanize.network.hh \
	humanize.time.hh \
	injector.hh \
	injector.bind.hh \
	intern_string.hh \
    is_utf8.hh \
    isc.hh \
    lnav_log.hh \
    lnav.gzip.hh \
    lrucache.hpp \
    math_util.hh \
    network.tcp.hh \
    opt_util.hh \
    paths.hh \
    result.h \
    string_util.hh \
    strnatcmp.h \
    time_util.hh

libbase_a_SOURCES = \
    auto_pid.cc \
    date_time_scanner.cc \
    fs_util.cc \
	humanize.cc \
	humanize.network.cc \
	humanize.time.cc \
	intern_string.cc \
    is_utf8.cc \
    isc.cc \
    lnav.gzip.cc \
    lnav_log.cc \
    network.tcp.cc \
    paths.cc \
    string_util.cc \
    strnatcmp.c \
    time_util.cc

check_PROGRAMS = \
    test_base

test_base_SOURCES = \
    humanize.file_size.tests.cc \
    humanize.network.tests.cc \
    humanize.time.tests.cc \
    intern_string.tests.cc \
    lnav.gzip.tests.cc \
    string_util.tests.cc \
    test_base.cc

test_base_LDADD = \
    libbase.a \
    ../fmtlib/libcppfmt.a \
    ../pcrepp/libpcrepp.a

TESTS = \
    test_base
