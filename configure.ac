# Process this file with autoconf to produce a configure script.
AC_INIT([dvipng], [0.2], [preview-latex-bugs@lists.sf.net])
AC_CONFIG_SRCDIR([dvipng.c])
AC_CONFIG_HEADER([config.h])

AC_SET_MAKE

# Checks for programs.
AC_PROG_CC
AC_PROG_INSTALL
AC_PATH_PROG(GS, gs, :)
AC_DEFINE_UNQUOTED(GS_PATH, "$GS", [Define as the path to GhostScript.])

# Checks for libraries.
AC_CHECK_LIB([gd], [gdImageCreate],,
             [AC_MSG_ERROR([cannot find/use libgd
This drawing library can be downloaded at http://www.boutell.com/gd])],
	     [-lm])
######
# Needed when statically linking against gd
AC_MSG_CHECKING([if -lgd needs -lm])
AC_TRY_LINK(, [gdImageCreate()], 
   AC_MSG_RESULT([no]),	
   LIBS="-lm $LIBS" 
   AC_MSG_RESULT([yes]))
######
AC_CHECK_LIB([png], [png_read_image],,
             [AC_MSG_ERROR([cannot find/use libpng])])
AC_CHECK_LIB([kpathsea], [kpse_set_progname],, 
             AC_MSG_ERROR([cannot find/use libkpathsea]))

# Checks for header files. 
AC_CHECK_HEADERS([gd.h png.h kpathsea/kpathsea.h],,
		 [AC_MSG_ERROR([cannot find/use $ac_header])])
AC_CHECK_HEADERS([fcntl.h sys/time.h])

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

# Documentation-related checks
AC_PATH_PROG(MAKEINFO, makeinfo, :)
MAKEINFO_CHECK_MACROS(acronym env option)
AC_PATH_PROG(INSTALL_INFO, install-info, :, $PATH /usr/sbin /sbin)

AC_CONFIG_FILES([Makefile])
AC_OUTPUT
