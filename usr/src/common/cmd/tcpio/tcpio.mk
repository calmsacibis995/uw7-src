#	copyright	"%c%"

#	Copyright (c) 1984, 1986, 1987, 1988, 1989 AT&T
#	  All Rights Reserved

#	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF AT&T
#	The copyright notice above does not evidence any
#	actual or intended publication of such source code.

#	Portions Copyright (c) 1988, Sun Microsystems, Inc.	
#	All Rights Reserved.				

#ident	"@(#)tcpio:tcpio.mk	1.5.2.3"
#ident "$Header$"

include $(CMDRULES)

# Makefile for tcpio componant (tcpio and rtcpio commands)

OWN = bin
GRP = bin

UTILDIR = $(USRLIB)/rtcpio

LDLIBS = -lgen -lgenIO -lcmd
MAINS = tcpio rtcpio

UTILS = vallvl lvldom

OBJECTS = tcpio.o tsec.o

all: $(MAINS) $(UTILS)

rtcpio:
	cp rtcpio.sh rtcpio

vallvl: vallvl.o
	$(CC) -o $@ $@.o $(LDFLAGS) $(LDLIBS) $(PERFLIBS)

lvldom: lvldom.o
	$(CC) -o $@ $@.o $(LDFLAGS) $(LDLIBS) $(PERFLIBS)

tcpio: $(OBJECTS) 
	$(CC) -o tcpio $(OBJECTS) $(LDFLAGS) $(LDLIBS) $(PERFLIBS)

tcpio.o: tcpio.c \
	$(INC)/stdio.h \
	$(INC)/sys/types.h \
	$(INC)/errno.h $(INC)/sys/errno.h \
	$(INC)/unistd.h \
	$(INC)/fcntl.h \
	$(INC)/memory.h \
	$(INC)/string.h \
	$(INC)/varargs.h \
	$(INC)/sys/stat.h \
	$(INC)/sys/mkdev.h \
	$(INC)/utime.h \
	$(INC)/pwd.h \
	$(INC)/grp.h \
	$(INC)/signal.h $(INC)/sys/signal.h \
	$(INC)/ctype.h \
	$(INC)/archives.h \
	$(INC)/locale.h \
	tcpio.h \
	$(INC)/sys/param.h \
	$(INC)/sys/mac.h \
	$(INC)/sys/acl.h \
	$(INC)/sys/secsys.h \
	$(INC)/priv.h \
	$(INC)/search.h \
	$(INC)/sys/utsname.h \
	$(INC)/ulimit.h \
	tsec.h \
	ttoc.h \
	tsec.h

tsec.o: tsec.c \
	$(INC)/sys/types.h \
	$(INC)/sys/param.h \
	$(INC)/sys/stat.h \
	$(INC)/stdio.h \
	$(INC)/ctype.h \
	$(INC)/fcntl.h \
	$(INC)/pwd.h \
	$(INC)/grp.h \
	$(INC)/errno.h $(INC)/sys/errno.h \
	$(INC)/sys/mac.h \
	$(INC)/sys/acl.h \
	$(INC)/sys/secsys.h \
	$(INC)/sys/utsname.h \
	$(INC)/priv.h \
	$(INC)/search.h \
	$(INC)/archives.h \
	tcpio.h \
	tsec.h \
	ttoc.h \
	tsec.h

vallvl.o: vallvl.c \
	$(INC)/sys/types.h \
	$(INC)/mac.h \
	$(INC)/stdlib.h

lvldom.o: lvldom.c \
	$(INC)/sys/types.h \
	$(INC)/mac.h \
	$(INC)/stdlib.h

clean:
	rm -f $(OBJECTS) vallvl.o lvldom.o

clobber: clean
	rm -f $(MAINS) $(UTILS)

lintit:
	$(LINT) $(LINTFLAGS) tcpio.c

install: all
	[ -d $(UTILDIR) ] || mkdir -p $(UTILDIR)
	$(CH)chmod 755 $(UTILDIR);
	$(CH)chown $(OWN) $(UTILDIR)
	$(INS) -f $(USRBIN) -m 0555 -u $(OWN) -g $(GRP) tcpio
	$(INS) -f $(USRBIN) -m 0555 -u $(OWN) -g $(GRP) rtcpio
	$(INS) -f $(UTILDIR) -m 0555 -u $(OWN) -g $(GRP) vallvl
	$(INS) -f $(UTILDIR) -m 0555 -u $(OWN) -g $(GRP) lvldom

#	These targets are useful but optional

partslist:
	@echo Makefile tcpio.c $(LOCALINCS) | tr ' ' '\012' | sort

productdir:
	@echo $(USRBIN) | tr ' ' '\012' | sort

product:
	@echo $(MAINS) | tr ' ' '\012' | \
	sed 's;^;$(USRBIN)/;'
