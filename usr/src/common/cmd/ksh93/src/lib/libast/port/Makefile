#ident	"@(#)ksh93:src/lib/libast/port/Makefile	1.1"
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

HEADERSRC = 

SOURCES = astconf.c astcopy.c astquery.c astwinsize.c \
	iblocks.c  touch.c

OBJECTS = $(SOURCES:.c=.o)

all:	$(OBJECTS)

clean:
	rm -f $(OBJECTS)

clobber:	clean

.c.o:
	$(CC) $(CCFLAGS) $(LOCALINC) $(GLOBALINC) -c $< && ln -f $@ ..
