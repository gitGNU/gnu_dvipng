# Makefile for dvipngk
version = 0.0

# paths.make -- installation directories.
#
# The compile-time paths are defined in kpathsea/paths.h, which is built
# from kpathsea/texmf.in and these definitions.  See kpathsea/INSTALL
# for how the various path-related files are used and created.

# Do not change prefix and exec_prefix in Makefile.in!
# configure doesn't propagate the change to the other Makefiles.
# Instead, give the -prefix/-exec-prefix options to configure.
# (See kpathsea/INSTALL for more details.) This is arguably
# a bug, but it's not likely to change soon.
prefix = /usr/local
exec_prefix = ${prefix}

# Architecture-dependent executables.
bindir = ${exec_prefix}/bin

# Architecture-independent executables.
scriptdir = $(bindir)

# Architecture-dependent files, such as lib*.a files.
libdir = ${exec_prefix}/lib

# Architecture-independent files.
datadir = /usr/share

# Header files.
includedir = ${prefix}/include

# GNU .info* files.
infodir = ${prefix}/info

# Unix man pages.
manext = 1
mandir = ${prefix}/man/man$(manext)

# TeX system-specific directories. Not all of the following are relevant
# for all programs, but it seems cleaner to collect everything in one place.

# The default paths are now in kpathsea/texmf.in. Passing all the
# paths to sub-makes can make the arg list too long on system V.
# Note that if you make changes below, you will have to make the
# corresponding changes to texmf.in or texmf.cnf yourself.

# The root of the main tree.
texmf = /usr/share/texmf

# The directory used by varfonts.
vartexfonts = /var/tmp/texfonts

# Regular input files.
texinputdir = $(texmf)/tex
mfinputdir = $(texmf)/metafont
mpinputdir = $(texmf)/metapost
mftinputdir = $(texmf)/mft

# dvips's epsf.tex, rotate.tex, etc. get installed here;
# ditto for dvilj's fonts support.
dvips_plain_macrodir = $(texinputdir)/plain/dvips
dvilj_latex2e_macrodir = $(texinputdir)/latex/dvilj

# mktex.cnf, texmf.cnf, etc.
web2cdir = $(texmf)/web2c

# The top-level font directory.
fontdir = $(texmf)/fonts

# Memory dumps (.fmt/.base/.mem).
fmtdir = $(web2cdir)
basedir = $(fmtdir)
memdir = $(fmtdir)

# Pool files.
texpooldir = $(web2cdir)
mfpooldir = $(texpooldir)
mppooldir = $(texpooldir)

# Where the .map files from fontname are installed.
fontnamedir = $(texmf)/fontname

# For dvips configuration files, psfonts.map, etc.
dvipsdir = $(texmf)/dvips

# For dvips .pro files, gsftopk's render.ps, etc.
psheaderdir = $(dvipsdir)

# If a font can't be found close enough to its stated size, we look for
# each of these sizes in the order given.  This colon-separated list is
# overridden by the envvar TEXSIZES, and by a program-specific variable
# (e.g., XDVISIZES), and perhaps by a config file (e.g., in dvips).
# This list must be sorted in ascending order.
default_texsizes = 300:600

# End of paths.make.
# common.make -- used by all Makefiles.
SHELL = /bin/sh

top_srcdir = .
srcdir = .

CC = gcc
CFLAGS = -g -O2 $(XCFLAGS) -Wall
CPPFLAGS =  $(XCPPFLAGS)
DEFS = -DHAVE_CONFIG_H $(XDEFS) -DDEBUG

# Kpathsea needs this for compiling, programs need it for linking.
LIBTOOL = ./klibtool # FIXME $(kpathsea_srcdir_parent)/klibtool

# You can change [X]CPPFLAGS, [X]CFLAGS, or [X]DEFS, but
# please don't change ALL_CPPFLAGS or ALL_CFLAGS.
# prog_cflags is set by subdirectories of web2c.
ALL_CPPFLAGS = $(DEFS) -I. -I$(srcdir) $(prog_cflags) \
  -I$(kpathsea_parent) -I$(kpathsea_srcdir_parent) $(CPPFLAGS)
ALL_CFLAGS = $(ALL_CPPFLAGS) $(CFLAGS) -c
compile = $(CC) $(ALL_CFLAGS)

.SUFFIXES: .c .o # in case the suffix list has been cleared, e.g., by web2c
.c.o:
	$(compile) $<

# Installation.
INSTALL = /usr/bin/install -c
INSTALL_PROGRAM = ${INSTALL}
INSTALL_SCRIPT = $(INSTALL_PROGRAM)
INSTALL_DATA = ${INSTALL} -m 644
INSTALL_LIBTOOL_LIBS = INSTALL_DATA='$(INSTALL_DATA)' $(LIBTOOL) install-lib
INSTALL_LIBTOOL_PROG = INSTALL_PROGRAM='$(INSTALL_PROGRAM)' $(LIBTOOL) install-prog

# Creating (symbolic) links.
LN = ln -s

# We use these for many things.
kpathsea_parent = ..
kpathsea_dir = $(kpathsea_parent)/kpathsea
kpathsea_srcdir_parent = $(top_srcdir)/..
kpathsea_srcdir = $(kpathsea_srcdir_parent)/kpathsea
kpathsea = /usr/lib/libkpathsea.a # FIXME $(kpathsea_dir)/libkpathsea.la

#M#ifeq ($(CC), gcc)
#M#XDEFS = -D__USE_FIXED_PROTOTYPES__ -Wall -Wpointer-arith $(warn_more)
#M#CFLAGS = -pipe -g $(XCFLAGS)
#M#endif
# End of common.make.
# programs.make -- used by Makefiles for executables only.

# Don't include $(CFLAGS), since ld -g under Linux forces
# static libraries, e.g., libc.a and libX*.a.
LDFLAGS =  $(XLDFLAGS)

# proglib is for web2c; 
# XLOADLIBES is for the installer.
LIBS = 
LOADLIBES = $(proglib) $(kpathsea) $(LIBS) -lm -lgd $(XLOADLIBES)

# May as well separate linking from compiling, just in case.
CCLD = $(CC)
link_command = $(CCLD) -o $@ $(LDFLAGS) 

# When we link with Kpathsea, have to take account that it might be a
# shared library, etc.
kpathsea_link = $(LIBTOOL) link $(link_command)
# End of programs.make.
prog_cflags = -DUNIX -DKPATHSEA

common_objects = 

program = dvipng
objects = dvipng.o color.o font.o io.o misc.o pk.o \
	set.o special.o papersiz.o pages.o
all: $(program)

$(program): $(objects) $(kpathsea)
	$(kpathsea_link) $(objects) $(LOADLIBES)

$(objects): dvipng.h

# tkpathsea.make -- target for remaking kpathsea.
makeargs = $(MFLAGS) CC='$(CC)' CFLAGS='$(CFLAGS)' $(XMAKEARGS)

# This is wrong: the library doesn't depend on kpsewhich.c or
# acconfig.h.  But what to do?
#$(kpathsea): $(kpathsea_srcdir)/*.c $(kpathsea_srcdir)/*.h \
#	     $(kpathsea_srcdir)/texmf.in $(top_srcdir)/../make/paths.make
#	cd $(kpathsea_dir) && $(MAKE) $(makeargs)
# End of tkpathsea.make.

install: 


# clean.make -- cleaning.
mostlyclean::
	rm -f *.o

clean:: mostlyclean
	rm -f $(program) $(programs) squeeze lib$(library).* $(library).a *.bad
	rm -f *.exe *.dvi *.lj

distclean:: extraclean clean
	rm -f Makefile
	rm -f config.status config.log config.cache c-auto.h
	rm -f stamp-auto stamp-tangle stamp-otangle

# End of rdepend.make.
dvipng.o: dvipng.c dvipng.h config.h commands.h
#dvipng.o: dvipng.c dvipng.h $(kpathsea_srcdir)/config.h c-auto.h \
# $(kpathsea_srcdir)/c-std.h \
# $(kpathsea_srcdir)/c-unistd.h \
# $(kpathsea_srcdir)/systypes.h \
# $(kpathsea_srcdir)/c-memstr.h $(kpathsea_srcdir)/c-errno.h \
# $(kpathsea_srcdir)/c-minmax.h \
# $(kpathsea_srcdir)/c-limits.h \
# $(kpathsea_srcdir)/c-proto.h \
# $(kpathsea_srcdir)/debug.h $(kpathsea_srcdir)/types.h $(kpathsea_srcdir)/lib.h \
# $(kpathsea_srcdir)/progname.h $(kpathsea_srcdir)/magstep.h $(kpathsea_srcdir)/proginit.h \
# $(kpathsea_srcdir)/tex-glyph.h $(kpathsea_srcdir)/tex-file.h $(kpathsea_srcdir)/tex-hush.h \
# $(kpathsea_srcdir)/tex-make.h $(kpathsea_srcdir)/c-vararg.h \
# config.h commands.h
