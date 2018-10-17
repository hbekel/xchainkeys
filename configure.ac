#                                               -*- Autoconf -*-
# Process this file with autoconf to produce a configure script.

AC_PREREQ([2.65])
AC_INIT([xchainkeys], [0.11], [h.bekel@googlemail.com])
AC_CONFIG_SRCDIR([src/xchainkeys.c])
AM_INIT_AUTOMAKE

# Checks for programs.
AC_PROG_CC
AC_PROG_SED

# Make sure headers and libs are found on BSD as well.
CFLAGS="-I/usr/local/include $CFLAGS"
CPPFLAGS="-I/usr/local/include $CPPFLAGS"
LDFLAGS="-L/usr/local/lib $LDFLAGS"

# Checks for headers.
AC_CHECK_HEADER(X11/Xlib.h,,
  AC_MSG_ERROR([Could not find X11/Xlib.h])
)

# Checks for libraries.
AC_CHECK_LIB(X11, XOpenDisplay,
  LIBS="$LIBS -lX11",
  AC_MSG_ERROR([Could not find XOpenDisplay in -lX11.])
)

# Create Makefiles
AC_OUTPUT(Makefile src/Makefile)
AC_OUTPUT
