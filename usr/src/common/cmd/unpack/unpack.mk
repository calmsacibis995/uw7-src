#	Copyright (c) 1984, 1986, 1987, 1988, 1989 AT&T
#	  All Rights Reserved

#	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF AT&T
#	The copyright notice above does not evidence any
#	actual or intended publication of such source code.

#ident	"@(#)unpack:unpack.mk	1.11.1.3"
#ident "$Header$"

include $(CMDRULES)

#	unpack (pcat) make file

OWN = bin
GRP = bin

LOCAL_LDLIBS = $(LDLIBS) -lgen
all: unpack

unpack: unpack.c \
	$(INC)/stdio.h $(INC)/fcntl.h \
	$(INC)/signal.h $(INC)/sys/signal.h \
	$(INC)/sys/types.h $(INC)/sys/stat.h \
	$(INC)/unistd.h $(INC)/locale.h \
	$(INC)/pfmt.h $(INC)/string.h \
	$(INC)/errno.h $(INC)/sys/errno.h \
	$(INC)/setjmp.h
	$(CC) $(CFLAGS) $(DEFLIST) -o $@ $@.c $(LDFLAGS) $(LDLIBS) $(LOCAL_LDLIBS) $(PERFLIBS)

pcat: unpack

install: all
	 $(INS) -f $(USRBIN) -m 0555 -u $(OWN) -g $(GRP) unpack
	-rm -f $(USRBIN)/pcat
	ln $(USRBIN)/unpack $(USRBIN)/pcat

clean:
	 rm -f unpack.o

clobber: clean
	 rm -f unpack

lintit:
	$(LINT) $(LINTFLAGS) unpack.c

# optional targets

SL = $(ROOT)/usr/src/common/cmd/unpack
RDIR = $(SL)
REL = current
CSID = -r`gsid unpack $(REL)`
MKSID = -r`gsid unpack.mk $(REL)`
LIST = lp

build: bldmk
	get -p $(CSID) s.unpack.c $(REWIRE) > $(RDIR)/unpack.c
bldmk:
	get -p $(MKSID) s.unpack.mk > $(RDIR)/unpack.mk
	rm -f $(RDIR)/pcat.mk
	ln $(RDIR)/unpack.mk $(RDIR)/pcat.mk

listing:
	pr unpack.mk unpack.c | $(LIST)
listmk: ; pr unpack.mk | $(LIST)

edit:
	get -e s.unpack.c

delta:
	delta s.unpack.c

mkedit: ; get -e s.unpack.mk
mkdelta: ; delta s.unpack.mk

delete: clobber
	rm -f unpack.c
