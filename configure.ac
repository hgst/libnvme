# Process this file with autoconf to produce a configure script.

#AC_PREREQ([2.69])
AC_INIT([libnvme], [0.0.1], [christophe.louargant@wdc.com, damien.lemoal@wdc.com])
AC_CONFIG_AUX_DIR([build-aux])
AC_CONFIG_MACRO_DIR([m4])
AC_CONFIG_HEADER([include/config.h])

AC_PREFIX_DEFAULT(/usr)

AM_INIT_AUTOMAKE([-Wall foreign subdir-objects])
AM_SILENT_RULES([yes])

AC_PROG_CC
AM_PROG_CC_C_O

m4_ifdef([AM_PROG_AR], [AM_PROG_AR])
m4_pattern_allow([AM_PROG_AR])
LT_INIT

ACX_PTHREAD
LIBS="$PTHREAD_LIBS $LIBS"
CFLAGS="$CFLAGS $PTHREAD_CFLAGS"
CC="$PTHREAD_CC"

# Checks for header files.
AC_CHECK_HEADERS([stddef.h stdint.h stdlib.h \
		  string.h inttypes.h \
		  unistd.h assert.h])
AC_CHECK_LIB(pciaccess, pci_device_probe, [], AC_MSG_ERROR([Couldn't find libpciaccess. Try installing libpciaccess]))
AC_CHECK_HEADER(pciaccess.h, [], [AC_MSG_ERROR([Couldn't find pciaccess.h. Try installing libpciaccess-devel])])
AC_CHECK_LIB(numa, mbind, [], AC_MSG_ERROR([Couldn't find libnuma. Try installing numactl-libs]))
AC_CHECK_HEADER(numaif.h, [], [AC_MSG_ERROR([Couldn't find numaif.h. Try installing numactl-devel])])

# CPU flags detection
if grep "^flags.* sse" /proc/cpuinfo > /dev/null; then
  have_sse=yes
fi
if grep "^flags.* sse2" /proc/cpuinfo > /dev/null; then
  have_sse2=yes
fi
if grep "^flags.* avx" /proc/cpuinfo > /dev/null; then
  have_avx=yes
fi

if test "$have_sse" = "yes"; then have_sse2=yes; fi
if test "$have_sse2" = "yes"; then
	AC_DEFINE(__SSE2__,1,[Define to enable SSE/SSE2 optimizations.])
fi
AM_CONDITIONAL(__SSE2__, test "$have_sse2" = "yes")

if test "$have_avx" = "yes"; then
        AC_DEFINE(__AVX__,1,[Define to enable AVX optimizations.])
fi
AM_CONDITIONAL(__AVX__, test "$have_avx" = "yes")

# Checks for library functions.
# AC_CHECK_FUNCS([memset])
AC_CONFIG_FILES([
        libnvme.pc
        Makefile
])

AC_OUTPUT
