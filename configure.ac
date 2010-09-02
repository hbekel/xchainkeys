#                                               -*- Autoconf -*-
# Process this file with autoconf to produce a configure script.

AC_PREREQ([2.65])
AC_INIT([xchainkeys], [0.1], [h.bekel@googlemail.com], [], [])
AM_INIT_AUTOMAKE([xchainkeys], [0.1])

# Checks for programs.
AC_PROG_CC

# Checks for libraries.
AC_CHECK_LIB(X11, XOpenDisplay,
  LIBS="$LIBS -lX11",
  AC_MSG_ERROR([Could not find XOpenDisplay in -lX11.])
)

# Create Makefiles
AC_OUTPUT(Makefile src/Makefile)
AC_OUTPUT
