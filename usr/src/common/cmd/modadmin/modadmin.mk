#ident	"@(#)modadmin:modadmin.mk	1.3"
#ident	"$Header$"

include $(CMDRULES)

#	Makefile for modadmin

OWN = root
GRP = sys

LDLIBS = -lgen

all:	modadmin modadmin.dy

modadmin:	modadmin.o
	$(CC) -o modadmin modadmin.o $(LDFLAGS) $(LDLIBS) $(ROOTLIBS)

modadmin.dy:	modadmin.o
	$(CC) -o modadmin.dy modadmin.o $(LDFLAGS) $(LDLIBS)

modadmin.o:	modadmin.c		\
		$(INC)/libgen.h		\
		$(INC)/locale.h		\
		$(INC)/pfmt.h		\
		$(INC)/stdio.h		\
		$(INC)/stdlib.h		\
		$(INC)/string.h		\
		$(INC)/sys/errno.h	\
		$(INC)/sys/mod.h	\
		$(INC)/time.h

clean:
	rm -f modadmin.o

clobber: clean
	rm -f modadmin

lintit:
	$(LINT) $(LINTFLAGS) modadmin.c

install:	all
	$(INS) -f $(SBIN) -m 0555 -u $(OWN) -g $(GRP) modadmin
	$(INS) -f $(SBIN) -m 0555 -u $(OWN) -g $(GRP) modadmin.dy
