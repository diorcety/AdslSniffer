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

AC_CHECK_PROGS(SDCC,[sdcc sdcc-sdcc],no)
if test x"$SDCC" = xno ; then
	AC_MSG_ERROR(Cannot find sdcc.)
fi
AC_SUBST([SDCC])

AC_CHECK_PROGS(SDAS8051,[sdas8051 sdcc-sdas8051],no)
if test x"$SDAS8051" = xno ; then
	AC_MSG_ERROR(Cannot find sdas8051.)
fi
AC_SUBST([SDAS8051])

AC_CHECK_PROGS(SDCCLIB,[sdcclib sdcc-sdcclib],no)
if test x"$SDCCLIB" = xno ; then
	AC_MSG_ERROR(Cannot find sdcclib.)
fi
AC_SUBST([SDCCLIB])

# Checks for libraries.
PKG_CHECK_MODULES([libusb], [libusb-1.0])


# Checks for header files.

# Checks for typedefs, structures, and compiler characteristics.

# Checks for library functions.

AC_OUTPUT(
	Makefile
	firmware/Makefile
	src/Makefile
	test/Makefile
	lib/Makefile
)
