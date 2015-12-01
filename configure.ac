dnl Process this file with autoconf to produce a configure script.
AC_REVISION($Revision: 1.0 $)dnl
AC_INIT(src/ascii.h)
AM_INIT_AUTOMAKE(slsnif, 0.4.4)
AC_LANG_C
dnl Checks for programs.
AC_ISC_POSIX
AC_PROG_CC
AC_PROG_INSTALL
AC_PROG_MAKE_SET
dnl Checks for libraries.

dnl Checks for header files.
AC_HEADER_STDC
AC_CHECK_HEADERS(fcntl.h unistd.h getopt.h time.h)
AC_CHECK_HEADERS(linux/kdev_t.h linux/major.h sys/timeb.h)

dnl Checks for typedefs, structures, and compiler characteristics.
AC_TYPE_PID_T

dnl Checks for library functions.
AC_TYPE_SIGNAL
AC_CHECK_FUNCS(select strdup getopt_long time ctime ftime getpt grantpt unlockpt ptsname)
AC_FUNC_SELECT_ARGTYPES
AM_CONFIG_HEADER(config.h)
AC_OUTPUT(Makefile src/Makefile)
