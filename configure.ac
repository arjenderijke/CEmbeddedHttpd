#                                               -*- Autoconf -*-
# Process this file with autoconf to produce a configure script.

AC_PREREQ([2.69])
AC_INIT([CEmbeddedHttpd], [0.0.1], [arjenderijke@gmail.com])
AM_INIT_AUTOMAKE
AC_CONFIG_SRCDIR([src/main.c])
AC_CONFIG_HEADERS([config.h])

# Checks for programs.
AC_PROG_CC
AC_PROG_INSTALL

# Checks for libraries.
have_microhttpd=no
AC_SEARCH_LIBS([MHD_start_daemon], [microhttpd], [have_microhttpd=yes])

# Checks for header files.
AC_CHECK_HEADERS([stdlib.h sys/socket.h])

if test "x${have_microhttpd}" = xyes; then
   AC_CHECK_HEADERS([microhttpd.h], [], [have_microhttpd=no])
fi

if test "x{have_microhttpd}" = xno; then
   AC_MSG_ERROR([
   ------------------------------
     The microhttpd library and header file are
     required.
   ------------------------------])
fi

# Checks for typedefs, structures, and compiler characteristics.
AC_TYPE_SIZE_T

# Checks for library functions.

AC_CONFIG_FILES([Makefile
                 src/Makefile])
AC_OUTPUT

echo \
"------------------------------

${PACKAGE_NAME} Version ${PACKAGE_VERSION}

Prefix: '${prefix}'
Compiler: '${CC} ${CFLAGS} ${CPPFLAGS}

Now type 'make @<:@<target>@:>@'
  where the optional <target> is:
    all            - build all binaries
    install        - install everything
------------------------------"
