#                                               -*- Autoconf -*-
# Process this file with autoconf to produce a configure script.
AC_PREREQ([2.59])
AC_INIT([dns_cache], [0.1], [])
AC_CONFIG_SRCDIR([src/dns_cache.c])
AC_CONFIG_HEADERS([config.h])


AM_INIT_AUTOMAKE([-Wall -Werror foreign])

LT_INIT
AC_PROG_CC
AC_PROG_LIBTOOL

# AC_PROG_CC will define CFLAGS to "-g -O2"
CFLAGS= "-g"

# Checks for libraries.
case "$host" in
*-apple-darwin*)
    CFLAGS+=" -L/opt/local/lib -I/opt/local/include"
    ;;
esac

PKG_CHECK_MODULES(GLIB, [glib-2.0], [], [AC_MSG_ERROR([glib need])])
PKG_CHECK_MODULES(EVENT, [libevent], [], [AC_MSG_ERROR([libevent need])])

# Checks for header files.
AC_CHECK_HEADERS([arpa/inet.h limits.h malloc.h netinet/in.h stdlib.h string.h sys/socket.h sys/time.h unistd.h])


# Checks for typedefs, structures, and compiler characteristics.
AC_HEADER_STDBOOL
AC_C_INLINE

# Checks for library functions.
AC_FUNC_MALLOC
AC_CHECK_FUNCS([memset socket strcasecmp strchr strtoul])

AC_CONFIG_FILES([
    Makefile
    src/Makefile
])
AC_OUTPUT
