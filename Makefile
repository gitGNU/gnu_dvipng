# Makefile for dvipngk
version = 0.0

CC = gcc
CFLAGS = -g -O2 -Wall
CPPFLAGS = -I. -DKPATHSEA -DDEBUG

# Linking
LOADLIBES = -lkpathsea -lgd -lm

# Installation.
bindir = /usr/bin
INSTALL = /usr/bin/install -c

objects = dvipng.o color.o draw.o font.o io.o misc.o pk.o \
	set.o special.o papersiz.o pagelist.o pagequeue.o

dvipng: $(objects)
	$(CC) $(LDFLAGS) $(objects) -o dvipng $(LOADLIBES) $(LDLIBS)

$(objects): dvipng.h config.h commands.h

install: dvipng
	$(INSTALL) dvipng $(bindir)

clean:
	rm -f *.o dvipng

distclean: clean
	rm -f Makefile
	rm -f config.status config.log config.cache c-auto.h
