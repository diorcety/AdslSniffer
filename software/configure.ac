#                                               -*- Autoconf -*-
# Process this file with autoconf to produce a configure script.

AC_PREREQ([2.69])
AC_INIT([AdslSniffer], [0.1], [diorcet.yann@gmail.com])
AM_INIT_AUTOMAKE
LT_INIT

# Checks for programs.
AC_PROG_CC

AC_LANG([C++])
AC_PROG_CXX
AX_CXX_COMPILE_STDCXX_11

# Checks for libraries.
PKG_CHECK_MODULES([LIBUSB], [libusb-1.0])


# Checks for header files.

# Checks for typedefs, structures, and compiler characteristics.

# Checks for library functions.

AC_OUTPUT(Makefile src/Makefile test/Makefile lib/Makefile)
