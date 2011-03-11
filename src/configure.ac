#                                               -*- Autoconf -*-
# Process this file with autoconf to produce a configure script.

AC_PREREQ([2.61])
AC_INIT([OverpassAPI], [0.6.0], [roland.olbricht@gmx.de])
AC_CONFIG_SRCDIR([backend/types.h])
AC_CONFIG_HEADERS([config.h])
AM_INIT_AUTOMAKE([-Wall -Werror foreign])

# Checks for programs.
AC_PROG_CC
AC_PROG_CXX

# Checks for libraries.
AC_CHECK_LIB([expat], [XML_Parse])
AC_CHECK_LIB([rt], [time])

# Checks for header files.
AC_TYPE_SIZE_T
AC_HEADER_DIRENT
AC_HEADER_STDC
AC_CHECK_HEADERS([fcntl.h locale.h stdint.h stdlib.h string.h sys/time.h unistd.h])

# Checks for typedefs, structures, and compiler characteristics.
AC_C_CONST
AC_HEADER_STDBOOL
AC_HEADER_TIME
AC_STRUCT_TM

# Checks for library functions.
AC_CHECK_FUNCS([clock_gettime ftruncate select])
AC_FUNC_CLOSEDIR_VOID
AC_FUNC_MALLOC
AC_FUNC_MMAP
AC_FUNC_SELECT_ARGTYPES
AC_FUNC_STAT

AC_CONFIG_FILES([Makefile bin/Makefile test-bin/Makefile])
AC_OUTPUT
