
#ident	"@(#)patch_p2:patch.mk	1.1"

include $(CMDRULES)

#	Makefile for patch

OWN = bin
GRP = bin

LOCAL_LDLIBS = $(LDLIBS) -lgen

all: patch

OBJECTS = patch.o inp.o util.o pch.o version.o

patch: patch.o inp.o util.o pch.o version.o
	$(CC) $(CFLAGS) -o patch $(OBJECTS) $(LDFLAGS) $(LDLIBS) $(ROOTLIBS)


install: all
	 $(INS) -f $(USRBIN) -m 0555 -u $(OWN) -g $(GRP) patch

clean:
	rm -f patch.o

clobber: clean
	rm -f patch

lintit:
	$(LINT) $(LINTFLAGS) patch.c

