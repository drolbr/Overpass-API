#                                               -*- Autoconf -*-
# Process this file with autoconf to produce a configure script.

AC_PREREQ([2.61])
AC_INIT([OverpassAPI], [0.7.52], [roland.olbricht@gmx.de])
AC_CONFIG_SRCDIR([template_db/file_blocks.h])
AC_CONFIG_HEADERS([config.h])
AC_CONFIG_MACRO_DIR([m4])
AM_INIT_AUTOMAKE([-Wall foreign subdir-objects])

# Checks for programs.
AM_PROG_AR
AC_PROG_CC
AC_PROG_CXX
AC_PROG_INSTALL
AC_PROG_LIBTOOL
AC_PROG_MAKE_SET

# Checks for libraries.
AC_CHECK_LIB([expat], [XML_Parse])
AC_SEARCH_LIBS([shm_open], [rt])

# Checks for header files.
AC_TYPE_MODE_T
AC_TYPE_PID_T
AC_TYPE_SIGNAL
AC_TYPE_SIZE_T
AC_HEADER_DIRENT
AC_HEADER_STDC
AC_CHECK_HEADERS([fcntl.h locale.h stdint.h stdlib.h string.h sys/time.h unistd.h])

# Checks for typedefs, structures, and compiler characteristics.
AC_C_CONST
AC_C_INLINE
AC_C_VOLATILE
AC_HEADER_STDBOOL
AC_HEADER_TIME
AC_STRUCT_TM

# Checks for library functions.
AC_FUNC_CLOSEDIR_VOID
AC_FUNC_MALLOC
AC_FUNC_MMAP
AC_FUNC_SELECT_ARGTYPES
AC_FUNC_STAT
AC_FUNC_STRFTIME
AC_CHECK_FUNCS([clock_gettime floor ftruncate getcwd munmap regcomp select shm_open socket sqrt strnlen])

AC_ARG_ENABLE([lz4],
              AS_HELP_STRING([--enable-lz4],[enable lz4 compression algorithm]),,
              [enable_lz4="no"])
AS_IF([test x"$enable_lz4" != "xno"], [want_lz4="yes"], [want_lz4="no"])

COMPRESS_LIBS="-lz"
AC_SUBST(COMPRESS_LIBS, ["$COMPRESS_LIBS"])

if test "$want_lz4" != "no"; then
  AC_CHECK_HEADER(lz4.h, [
    AC_CHECK_LIB(lz4, LZ4_compress, [
      AC_DEFINE(HAVE_LZ4, 1, [Define if you have lz4 library])
      COMPRESS_LIBS="$COMPRESS_LIBS -llz4"
    ], [
      if test "$want_lz4" = "yes"; then
	    AC_ERROR([Can't build with lz4 support: liblz4 not found])
      fi
    ])
  ], [
    if test "$want_lz4" = "yes"; then
      AC_ERROR([Can't build with lz4 support: lz4.h not found])
    fi
  ])
fi

AC_SUBST(COMPRESS_LIBS, ["$COMPRESS_LIBS"])

AC_CONFIG_FILES([Makefile test-bin/Makefile])
AC_OUTPUT
AC_SYS_LARGEFILE


