CC = gcc
CFLAGS = -g -O2 -Wall
CPPFLAGS = -I. -DKPATHSEA -DDEBUG

LOADLIBES = -lkpathsea -lgd -lm

bindir = /usr/bin
INSTALL = /usr/bin/install -c

objects = dvipng.o color.o draw.o dvi.o font.o io.o misc.o pk.o \
	set.o special.o papersiz.o pagequeue.o vf.o

dvipng: $(objects)
	$(CC) $(LDFLAGS) $(objects) -o dvipng $(LOADLIBES) $(LDLIBS)

$(objects): dvipng.h commands.h

install: dvipng
	$(INSTALL) dvipng $(bindir)

clean:
	rm -f *.o dvipng

distclean: clean
	rm -f Makefile
	rm -f config.status config.log config.cache c-auto.h
