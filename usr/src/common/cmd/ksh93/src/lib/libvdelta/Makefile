#ident	"@(#)ksh93:src/lib/libvdelta/Makefile	1.2"
#  old kpv delta/update/malloc library

include $(LIBRULES)

INSTALLROOT = ../../..

SOURCES = vddelta.c vdio.c vdupdate.c

CCFLAGS= -O $(CC.PIC)

OBJECTS = $(SOURCES:.c=.o)

LOCALINC = -I$(INSTALLROOT)/src/lib/libast/include \
	-I $(INSTALLROOT)/src/lib/libast \
	-I $(INSTALLROOT)/src/lib/libvdelta

all:	libvdelta.a

clean: 
	rm -f $(OBJECTS)

clobber:	clean
	rm -f libvdelta.a

libvdelta.a:	vdelhdr.h vdelta.h $(OBJECTS)
	$(AR) $(ARFLAGS) libvdelta.a $(OBJECTS)

.c.o:
	$(CC) $(CCFLAGS) $(LOCALINC) $(GLOBALINC) -D_PACKAGE_ast -c $<
