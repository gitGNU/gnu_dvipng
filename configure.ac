# configure.ac

#************************************************************************
#
#  Part of the dvipng distribution
#
#  This program is free software; you can redistribute it and/or modify
#  it under the terms of the GNU General Public License as published by
#  the Free Software Foundation; either version 2 of the License, or
#  (at your option) any later version.
#
#  This program is distributed in the hope that it will be useful, but
#  WITHOUT ANY WARRANTY; without even the implied warranty of
#  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
#  General Public License for more details.
#
#  You should have received a copy of the GNU General Public License
#  along with this program; if not, write to the Free Software
#  Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA
#  02111-1307, USA.
#
#  Copyright (C) 2002-2004 Jan-Åke Larsson
#
#************************************************************************

# Process this file with autoconf to produce a configure script.
AC_INIT([dvipng], [1.3], [dvipng-devel@lists.sf.net])
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
AC_PROG_LN_S
AC_PATH_PROG(GS, gs, :)
AC_DEFINE_UNQUOTED(GS_PATH, "$GS", [Define as the path to GhostScript.])

# Checks for libraries.
AC_CHECK_LIB(m, pow)
AC_CHECK_LIB(z, deflate)
AC_CHECK_LIB(gen, basename)
AC_CHECK_LIB([png], [png_read_image],,
             [AC_MSG_ERROR([cannot find/use libpng])])
AC_CHECK_LIB([gd], [gdImageCreate],,
             [AC_MSG_ERROR([cannot find/use libgd
This drawing library can be downloaded at http://www.boutell.com/gd])])
AC_CHECK_LIB([kpathsea], [kpse_set_progname],, 
             AC_MSG_ERROR([cannot find/use libkpathsea]))
AC_CHECK_LIB([t1], [T1_InitLib])
PSFONTS_O=""
if test "$ac_cv_lib_t1_T1_InitLib" = yes; then
  PSFONTS_O="t1.o"
fi
AC_SUBST(PSFONTS_O)

# Checks for header files. 
AC_CHECK_HEADERS([gd.h png.h kpathsea/kpathsea.h],,
		 [AC_MSG_ERROR([cannot find/use $ac_header])])
AC_CHECK_HEADERS([t1lib.h])
AC_CHECK_FT2(,[CFLAGS="$FT2_CFLAGS $CFLAGS"
	LIBS="$FT2_LIBS $LIBS"
	PSFONTS_O="$PSFONTS_O ft.o"	
	AC_DEFINE(HAVE_FT2, 1, [Define to 1 if you have freetype2])
	ac_have_freetype2="yes"],
	[ac_have_freetype2="no"])
if test -n "$PSFONTS_O"; then
  PSFONTS_O="$PSFONTS_O enc.o fontmap.o tfm.o"
fi
AC_HEADER_SYS_WAIT
AC_CHECK_HEADERS([fcntl.h sys/time.h inttypes.h])

# Checks for typedefs, structures, and compiler characteristics.
AC_C_CONST
AC_TYPE_PID_T
AC_TYPE_SIZE_T
AC_HEADER_TIME
AC_HEADER_STDBOOL
AC_HAS_KPSE_ENC_FORMATS
AC_CHECK_TYPES([int64_t, long long])

# Checks for library functions.
AC_FUNC_ALLOCA
AC_FUNC_FORK
AC_FUNC_MALLOC
AC_HEADER_STDC
AC_FUNC_MMAP
AC_FUNC_STRTOD
AC_FUNC_VPRINTF
AC_CHECK_FUNCS([dup2 memset munmap pow putenv strchr strrchr strtol])
if test "$enable_timing" = "yes"; then
  AC_CHECK_FUNCS([ftime gettimeofday])
fi
AC_CHECK_FUNCS([gdImagePngEx gdImageCreateTrueColor gdImageTrueColorToPalette gdImageGif])
if test "$ac_cv_func_gdImageGif" = "yes"; then
   INSTALL_BIN_TARGET="install-dvigif"
else
   INSTALL_BIN_TARGET="install-dvipng"
fi
AC_SUBST(INSTALL_BIN_TARGET)

# Documentation-related checks
AC_PATH_PROG(MAKEINFO, makeinfo, :)
MAKEINFO_CHECK_MACROS(acronym env option)
AC_PATH_PROG(INSTALL_INFO, install-info, :, $PATH /usr/sbin /sbin)

# SELFAUTO
AC_ARG_ENABLE(selfauto-set, 
  AC_HELP_STRING([--enable-selfauto-set],
  [This option will make the final binary explicitly set the
   $SELFAUTO... variables to make it look as dvipng is installed in the
   main texmf tree, even if it isn't. This is necessary when texmf.cnf
   uses $SELFAUTO... variables and dvipng is not installed in the texmf
   tree. Otherwise, dvipng may not be able to find virtual fonts, or 
   psfonts.map.]),
  [ if test "$enableval" = yes ; then
      AC_MSG_CHECKING([for \$SELFAUTOLOC])
      SELFAUTOLOC=`kpsewhich -expand-var=\\\$SELFAUTOLOC`
      AC_DEFINE_UNQUOTED(ENV_SELFAUTOLOC,["SELFAUTOLOC=$SELFAUTOLOC"],
	[The environment setting for $SELFAUTOLOC])
      AC_MSG_RESULT([$SELFAUTOLOC])
      AC_MSG_CHECKING([for \$SELFAUTODIR])
      SELFAUTODIR=`kpsewhich -expand-var=\\\$SELFAUTODIR`
      AC_DEFINE_UNQUOTED(ENV_SELFAUTODIR,["SELFAUTODIR=$SELFAUTODIR"],
	[The environment setting for $SELFAUTODIR])
      AC_MSG_RESULT([$SELFAUTODIR])
      AC_MSG_CHECKING([for \$SELFAUTOPARENT])
      SELFAUTOPARENT=`kpsewhich -expand-var=\\\$SELFAUTOPARENT`
      AC_DEFINE_UNQUOTED(ENV_SELFAUTOPARENT,["SELFAUTOPARENT=$SELFAUTOPARENT"],
	[The environment setting for $SELFAUTOPARENT])
      AC_MSG_RESULT([$SELFAUTOPARENT])
    fi ],
  [AC_MSG_CHECKING([for texmf.cnf])
  TEXMF_CNF=`kpsewhich texmf.cnf`
  AC_MSG_RESULT([$TEXMF_CNF])
  AC_MSG_CHECKING([for psfonts.map])
  cp `which kpsewhich` .
  PSFONTS_MAP=`./kpsewhich psfonts.map`
  rm -f ./kpsewhich
  if test -n "$PSFONTS_MAP"; then
    AC_MSG_RESULT([$PSFONTS_MAP])
  else
    AC_MSG_RESULT([not found from outside the texmf tree])
    AC_MSG_CHECKING([for \$SELFAUTO in texmf.cnf])
    if grep SELFAUTO "$TEXMF_CNF" > /dev/null 2> /dev/null; then
      AC_MSG_RESULT([yes
***************************************************************
texmf.cnf is using \$SELFAUTO... variables. If you are going to
install dvipng outside the texmf tree, you may need to use 
--enable-selfauto-set, type ./configure --help for help.
***************************************************************])
    else
      AC_MSG_RESULT([no])
    fi
  fi])

AC_MSG_RESULT([
** Configuration summary for $PACKAGE_STRING:

   The -d (debug) switch is enabled:        $enable_debug
   Your gd is new enough to enable
        the -z (compression) switch:        $ac_cv_func_gdImagePngEx
   Your gd is new enough to enable
        the --truecolor switch:             $ac_cv_func_gdImageCreateTrueColor
   Your gd is new enough to enable
        gif support (dvigif):               $ac_cv_func_gdImageGif
   FreeType font rendering available:       $ac_have_freetype2
   T1lib font rendering available:          $ac_cv_lib_t1_T1_InitLib
])

AC_CONFIG_HEADER([config.h])
AC_CONFIG_FILES([Makefile])
AC_OUTPUT
