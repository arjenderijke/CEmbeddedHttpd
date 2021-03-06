#                                               -*- Autoconf -*-
# Process this file with autoconf to produce a configure script.

AC_PREREQ([2.68])
AC_INIT([CEmbeddedHttpd], [0.0.1], [arjenderijke@gmail.com])
AM_INIT_AUTOMAKE
LT_PREREQ([2.4.2])
LT_INIT
AC_CONFIG_SRCDIR([src/main.c])
AC_CONFIG_HEADERS([config.h])
AC_CONFIG_MACRO_DIR([m4])

# Checks for programs.
AC_PROG_CC
AC_PROG_INSTALL
AM_PROG_CC_C_O

# Check for pkg-config version
PKG_PROG_PKG_CONFIG([0.26])

# Checks for libraries.
have_microhttpd=no
AC_SEARCH_LIBS([MHD_start_daemon], [microhttpd], [have_microhttpd=yes])

# Checks for header files.
AC_CHECK_HEADERS([stdlib.h sys/socket.h])
#AC_CHECK_HEADER_STDBOOL

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

have_liburiparser=yes
AC_ARG_ENABLE(liburiparser,
	AS_HELP_STRING([--with-liburiparser=DIR],
		[liburiparser library is installed in DIR]),
	have_liburiparser="$withval")

if test "x$have_liburiparser" = xyes; then
      PKG_CHECK_MODULES([URIPARSER], [liburiparser],
      		[have_liburiparser=yes],
		[have_liburiparser=no; why_have_liburiparser="(liburiparser not found)"])
fi

URIPARSER_MISSING="Please install uriparser 0.6.4 or later."

AC_CHECK_LIB(uriparser, uriParseUriA,, AC_MSG_ERROR(${URIPARSER_MISSING}))
AC_CHECK_HEADER(uriparser/Uri.h,, AC_MSG_ERROR(${URIPARSER_MISSING}))

#if test "x${have_liburiparser}" = xyes; then
#   AC_CHECK_HEADERS([uriparser/Uri.h], [], [have_liburiparser=no])
#fi

#AC_SEARCH_LIBS([uriParseUriA], [liburiparser], [have_liburiparser=yes])

if test "x{have_liburiparser}" = xno; then
   AC_MSG_ERROR([
   ------------------------------
     The liburiparser library and header file are
     required.
   ------------------------------])
fi

have_cunit=no
AC_SEARCH_LIBS([CU_registry_initialized], [cunit], [have_cunit=yes])

AM_CONDITIONAL([HAVE_UNIT], [test "x${have_cunit}" = xyes])
if test "x${have_cunit}" = xyes; then
   AC_DEFINE([HAVE_UNIT], 1, [unittests enabled])
fi

have_libcurl=yes
AC_ARG_ENABLE(libcurl,
	AS_HELP_STRING([--with-libcurl=DIR],
		[libcurl library is installed in DIR]),
	have_libcurl="$withval")

if test "x$have_libcurl" = xyes; then
      PKG_CHECK_MODULES([CURL], [libcurl],
      		[have_libcurl=yes],
		[have_libcurl=no; why_have_libcurl="(libcurl not found)"])
fi

LIBCURL_MISSING="Please install libcurl 3 or later."

AC_CHECK_LIB(curl, curl_version,, AC_MSG_ERROR(${LIBCURL_MISSING}))
AC_CHECK_HEADER(curl/curl.h,, AC_MSG_ERROR(${LIBCURL_MISSING}))

AM_CONDITIONAL([HAVE_CURL], [test "x${have_libcurl}" = xyes])
if test "x${have_libcurl}" = xyes; then
   AC_DEFINE([HAVE_CURL], 1, [libcurl available])
fi

AC_ARG_ENABLE([debug],
	[  --enable-debug enable debug output],
	[debug=${enableval}], [debug=no])

if test "x${debug}" = xyes; then
   AC_DEFINE([DEBUG], 1, [debug output enabled])
fi

# Checks for typedefs, structures, and compiler characteristics.
AC_TYPE_SIZE_T

# Checks for library functions.

AC_CONFIG_FILES([Makefile
		 common/Makefile
		 libexamples/include/Makefile
		 libexamples/Makefile
		 libqueryhandler/include/Makefile
		 libqueryhandler/unittest/Makefile
		 libqueryhandler/Makefile
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
