# Process this file with autoconf to produce a configure script.
AC_INIT([dvipng], [0.6], [preview-latex-bugs@lists.sf.net])
AC_CONFIG_SRCDIR([dvipng.c])

AC_ARG_ENABLE(debug, 
  AC_HELP_STRING([--disable-debug],[Compile without debug (-d) option]), 
    [ if test "$enableval" = yes ; then
      AC_DEFINE(DEBUG, 1, [Define as 1 to get the debug (-d) option.])
      fi ],	
    [ enable_debug="yes";
      AC_DEFINE(DEBUG, 1, [Define as 1 to get the debug (-d) option.])])
AC_ARG_ENABLE(timing, 
  AC_HELP_STRING([--enable-timing],[Output execution time of dvipng]), 
   [ if test "$enableval" = yes ; then
     AC_DEFINE(TIMING, 1, [Define as 1 to get execution time output.])
     fi ])

# Checks for programs.
AC_SET_MAKE
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
AC_CHECK_FT2(,[CFLAGS="$FT2_CFLAGS $CFLAGS"
	LIBS="$FT2_LIBS $LIBS"
	FT_O="ft.o tfm.o psfontmap.o enc.o"	
	AC_DEFINE(HAVE_FT2, 1, [Define to 1 if you have freetype2])],
	[FT_O=""])
AC_SUBST(FT_O)
AC_HEADER_SYS_WAIT
AC_CHECK_HEADERS([fcntl.h sys/time.h])

# Checks for typedefs, structures, and compiler characteristics.
AC_C_CONST
AC_TYPE_PID_T
AC_HEADER_TIME

# Checks for library functions.
AC_FUNC_ALLOCA
AC_FUNC_FORK
AC_FUNC_MALLOC
AC_HEADER_STDC
AC_FUNC_MMAP
AC_FUNC_STRTOD
AC_FUNC_VPRINTF
AC_CHECK_FUNCS([dup2 strchr strrchr])
if test "$enable_timing" = "yes"; then
  AC_CHECK_FUNCS([ftime gettimeofday])
fi
AC_CHECK_FUNCS([gdImagePngEx gdImageCreateTrueColor gdImageTrueColorToPalette])

# Documentation-related checks
AC_PATH_PROG(MAKEINFO, makeinfo, :)
MAKEINFO_CHECK_MACROS(acronym env option)
AC_PATH_PROG(INSTALL_INFO, install-info, :, $PATH /usr/sbin /sbin)

# Dvips config files
if test "x$FT_O" != "x"; then
  AC_PATH_PROG(KPSEWHICH, kpsewhich, no)
  if test "$KPSEWHICH" != "no"; then
    AC_MSG_CHECKING([for ps2pk.map])
    PS2PK=`$KPSEWHICH psfonts.map | sed s%base/psfonts.map%% `config/ps2pk.map
    if test -e $PS2PK; then
      AC_DEFINE(HAVE_PS2PK_MAP, 1, 
		[Define to 1 if you have ps2pk.map where expected])
      AC_MSG_RESULT([$PS2PK])
    else
      AC_MSG_RESULT([no, this may inhibit your PS font selection])
    fi
  fi
fi

AC_MSG_RESULT([
** Configuration summary for $PACKAGE_STRING:

   The -d (debug) switch is enabled:        $enable_debug
   Your gd is new enough to enable
        the -z (compression) switch:        $ac_cv_func_gdImagePngEx
   Your gd is new enough to enable
        the -truecolor switch:              $ac_cv_func_gdImageCreateTrueColor
])

AC_CONFIG_HEADER([config.h])
AC_CONFIG_FILES([Makefile])
AC_OUTPUT
