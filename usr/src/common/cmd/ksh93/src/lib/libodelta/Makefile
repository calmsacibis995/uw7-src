#ident	"@(#)ksh93:src/lib/libodelta/Makefile	1.2"
#  old kpv delta/update/malloc library

include $(LIBRULES)

INSTALLROOT = ../../..

SOURCES = delta.c mtchstring.c suftree.c update.c

CCFLAGS = -O $(CC.PIC)

OBJECTS = $(SOURCES:.c=.o)

LOCALINC = -I$(INSTALLROOT)/src/lib/libast/include \
	-I $(INSTALLROOT)/src/lib/libast

all:	libodelta.a

clean: 
	rm -f $(OBJECTS)

clobber:	clean
	rm -f libodelta.a

libodelta.a:	update.h suftree.h $(OBJECTS)
	$(AR) $(ARFLAGS) libodelta.a $(OBJECTS)

.c.o:
	$(CC) $(CCFLAGS) $(LOCALINC) $(GLOBALINC) -c $<
