#ident	"@(#)ksh93:src/lib/libast/stdio/Makefile	1.1"
#  ast library make

include $(LIBRULES)

CCFLAGS = -O $(CC.PIC)

INSTALLROOT = ../../../..

LOCALINC = -I$(INSTALLROOT)/src/lib/libast/include \
	-I$(INSTALLROOT)/src/lib/libast/obsolete \
	-I$(INSTALLROOT)/src/lib/libast/sfio \
	-I$(INSTALLROOT)/src/lib/libast/misc \
	-I$(INSTALLROOT)/src/lib/libast/dir \
	-I$(INSTALLROOT)/src/lib/libast/re \
	-I$(INSTALLROOT)/src/lib/libast/hash \
	-I$(INSTALLROOT)/src/lib/libast/string \
	-I$(INSTALLROOT)/src/lib/libast/man \
	-I$(INSTALLROOT)/src/lib/libast

HEADERSRC = tm.h

SOURCES = stdgets.c stdopen.c stdprintf.c stdscanf.c stdsprnt.c \
	stdvbuf.c stdvsnprnt.c stdvsprnt.c stdvsscn.c

OBJECTS = $(SOURCES:.c=.o)

all:	$(OBJECTS)

clean:
	rm -f $(OBJECTS)

clobber:	clean

.c.o:
	$(CC) $(CCFLAGS) -D_PACKAGE_ast $(LOCALINC) $(GLOBALINC) -c $< && \
	ln -f $@ ..
