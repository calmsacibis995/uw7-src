#copyright	"%c%"
#ident	"@(#)pkg.mk	15.1"

include $(LIBRULES)

#	Makefile for libpkg

LOCALINC =  -Ihdrs

MAKEFILE = libpkg.mk

LIBRARY = libpkg.a

SOURCES = $(OBJECTS:.o=.c)

OBJECTS =  canonize.o ckparam.o ckvolseq.o cvtpath.o devtype.o dstream.o \
	gpkglist.o gpkgmap.o isdir.o logerr.o mappath.o pkgactkey.o \
	pkgexecl.o pkgexecv.o pkgmount.o pkgserid.o pkgtrans.o pkgxpand.o \
	ppkgmap.o privent.o progerr.o putcfile.o rrmdir.o runcmd.o \
	srchcfile.o tputcfent.o verify.o

all:		$(LIBRARY)

$(LIBRARY): $(OBJECTS)
	$(AR) $(ARFLAGS) $(LIBRARY) $(OBJECTS)

clean:
	rm -f $(OBJECTS)

clobber: clean
	rm -f $(LIBRARY)


