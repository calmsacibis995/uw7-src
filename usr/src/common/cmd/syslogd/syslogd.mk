#	Copyright (c) 1984, 1986, 1987, 1988, 1989 AT&T
#	  All Rights Reserved

#	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF AT&T
#	The copyright notice above does not evidence any
#	actual or intended publication of such source code.

#ident	"@(#)syslogd.mk	1.4"
#ident "$Header$"

include $(CMDRULES)

#
# syslogd.mk:
# makefile for syslogd(1M) daemon
#

OWN = root
GRP = sys

ELFLIBS = -lnsl
COFFLIBS = -lnsl_s -lc_s
CATNAME = $(USRLIB)/locale/C/LC_MESSAGES

all: syslogd

syslogd: syslogd.o
	if [ x$(CCSTYPE) = xCOFF ] ; \
	then \
	 	$(CC) -o syslogd syslogd.o $(LDFLAGS) $(LDLIBS) $(SHLIBS) $(COFFLIBS) ; \
	else \
	 	$(CC) -o syslogd syslogd.o $(LDFLAGS) $(LDLIBS) $(SHLIBS) $(ELFLIBS) ; \
	fi

local: syslogd.str
	mkmsgs -o syslogd.str uxsyslogd

syslogd.o: syslogd.c \
	$(INC)/errno.h $(INC)/sys/errno.h \
	$(INC)/stdio.h \
	$(INC)/ctype.h \
	$(INC)/signal.h $(INC)/sys/signal.h \
	$(INC)/string.h \
	$(INC)/netconfig.h \
	$(INC)/netdir.h \
	$(INC)/tiuser.h \
	$(INC)/utmp.h \
	$(INC)/sys/param.h \
	$(INC)/sys/fcntl.h \
	$(INC)/sys/stropts.h \
	$(INC)/sys/syslog.h \
	$(INC)/sys/strlog.h \
	$(INC)/sys/types.h \
	$(INC)/sys/stat.h \
	$(INC)/sys/time.h \
	$(INC)/sys/utsname.h \
	$(INC)/sys/poll.h \
	$(INC)/sys/wait.h

install: all local
	$(INS) -f $(USRSBIN) -m 0100 -u $(OWN) -g $(GRP) syslogd
	$(INS) -f $(ETC) -m 0444 -u $(OWN) -g $(GRP) syslog.conf
	[ -d $(CATNAME) ] || mkdir -p $(CATNAME)
	$(INS) -f $(CATNAME) -m 0444 uxsyslogd


clean:
	-rm -f syslogd.o uxsyslogd

clobber: clean
	-rm -f syslogd

lintit:
	$(LINT) $(LINTFLAGS) syslogd

