#ident "@(#)isdnmon.mk	28.1"
#ident "$Header$"

#
# isdnmon.mk: makefile for isdnmon port monitor 
#
#
#	Copyright (C) The Santa Cruz Operation, 1996
#	This Module contains Proprietary Information of
#	The Santa Cruz Operation, and should be treated as Confidential.
#

include $(CMDRULES)

OWN = root
GRP = sys


LDLIBS = -lnsl

SOURCES = isdnmon.c

OBJS = isdnmon.o

INSDIR = $(USRLIB)/saf

all: isdnmon

isdnmon: $(OBJS)
	$(CC) -o $@ $(OBJS) $(LDLIBS)

isdnmon.o: isdnmon.c \
	$(INC)/stdio.h \
	$(INC)/ctype.h \
	$(INC)/string.h \
	$(INC)/sys/types.h \
	$(INC)/sys/stropts.h \
	$(INC)/sys/poll.h \
	$(INC)/sac.h \
	$(INC)/stdlib.h \
	$(INC)/fcntl.h \
	$(INC)/unistd.h \
	$(INC)/errno.h \
	$(INC)/sys/scoisdn.h \
	$(INC)/cs.h 

install: all $(INSDIR)
	$(INS) -o -f $(INSDIR) -u $(OWN) -g $(GRP) isdnmon

$(INSDIR):
	[ -d $@ ] || mkdir -p $@
	$(CH)chown $(OWN) $@
	$(CH)chgrp $(GRP) $@

clean:
	-rm -f *.o

clobber: clean
	-rm -f isdnmon

lintit:
	$(LINT) $(LINTFLAGS) $(SOURCES)




