#ident	"@(#)Makefile	11.1"

SHELL	= /bin/sh

LOCALDEF=	-I. -Dsv4 -DPOSIX -D_POSIX_SOURCE -DJOB_CONTROL -DMOUSE

include $(CMDRULES)

LIB =		libpico.a

build:		$(LIB)

OBJS =		ansi.o \
		attach.o \
		basic.o \
		bind.o \
		browse.o \
		buffer.o \
		composer.o \
		display.o \
		file.o \
		fileio.o \
		line.o \
		osdep.o \
		pico.o \
		random.o \
		region.o \
		search.o \
		spell.o \
		tinfo.o \
		window.o \
		word.o

$(LIB):		$(OBJS)
		$(AR) ruv $(LIB) $(OBJS)

clean:
		rm -f $(OBJS)

clobber:	clean
		rm -f $(LIB)

