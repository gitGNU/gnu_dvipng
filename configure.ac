# Process this file with autoconf to produce a configure script.
AC_INIT([dvipng], [0.1], [preview-latex-bugs@lists.sf.net])
AC_CONFIG_SRCDIR([dvipng.c])
AC_CONFIG_HEADER([config.h])

# Checks for programs.
AC_PROG_CC
AC_PROG_INSTALL

# Checks for libraries.
AC_CHECK_LIB([gd], [gdImageCreate],, 
		   AC_MSG_ERROR([cannot find/use libgd]))
AC_CHECK_LIB([kpathsea], [kpse_set_progname],, 
			 AC_MSG_ERROR([cannot find/use libkpathsea]))

# Checks for header files.
AC_CHECK_HEADERS([fcntl.h stdint.h sys/time.h])

# Checks for typedefs, structures, and compiler characteristics.
AC_C_CONST
AC_HEADER_TIME

# Checks for library functions.
AC_FUNC_MALLOC
AC_HEADER_STDC
AC_FUNC_MMAP
AC_FUNC_STRTOD
AC_FUNC_VPRINTF
AC_CHECK_FUNCS([ftime gettimeofday strrchr])

AC_CONFIG_FILES([Makefile])
AC_OUTPUT
