#	Copyright (c) 1990, 1991 UNIX System Laboratories, Inc.
#	Copyright (c) 1984, 1986, 1987, 1988, 1989, 1990 AT&T
#	  All Rights Reserved

#	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF
#	UNIX System Laboratories, Inc.
#	The copyright notice above does not evidence any
#	actual or intended publication of such source code.

#	Copyright (c) 1990, 1991 UNIX System Laboratories, Inc.
#	Copyright (c) 1984, 1986, 1987, 1988, 1989, 1990 AT&T
#	  All Rights Reserved

#	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF
#	UNIX System Laboratories, Inc.
#	The copyright notice above does not evidence any
#	actual or intended publication of such source code.

#ident	"@(#)mp.cmds:common/cmd/priocntl/priocntl.mk	1.5"
#ident  "$Header$"

include $(CMDRULES)
CFLAGS= 

CLASSDIR = $(USRLIB)/class
LDLIBS = -lgen

SOURCES =  fppriocntl.c subr.c tspriocntl.c priocntl.c fcpriocntl.c
OBJECTS = $(SOURCES:.c=.o)

all: priocntl classes

priocntl: priocntl.o subr.o
	$(CC) priocntl.o subr.o -o priocntl $(LDFLAGS) $(LDLIBS) $(SHLIBS)

classes: FPpriocntl TSpriocntl FCpriocntl

FPpriocntl: fppriocntl.o subr.o
	$(CC) fppriocntl.o subr.o -o FPpriocntl $(LDFLAGS) $(LDLIBS) $(SHLIBS)

FCpriocntl: fcpriocntl.o subr.o
	$(CC) fcpriocntl.o subr.o -o FCpriocntl $(LDFLAGS) $(LDLIBS) $(SHLIBS)

TSpriocntl: tspriocntl.o subr.o
	$(CC) tspriocntl.o subr.o -o TSpriocntl $(LDFLAGS) $(LDLIBS) $(SHLIBS)

priocntl.o: priocntl.c \
	$(INC)/stdio.h \
	$(INC)/stdlib.h \
	$(INC)/string.h \
	$(INC)/search.h \
	$(INC)/unistd.h \
	$(INC)/sys/types.h \
	$(INC)/dirent.h \
	$(INC)/fcntl.h $(INC)/sys/fcntl.h \
	$(INC)/sys/procset.h \
	$(INC)/sys/priocntl.h \
	$(INC)/sys/procfs.h \
	$(INC)/macros.h \
	$(INC)/errno.h $(INC)/sys/errno.h \
	$(INC)/priv.h \
	priocntl.h

fppriocntl.o: fppriocntl.c

fcpriocntl.o: fcpriocntl.c

tspriocntl.o: tspriocntl.c 

subr.o: subr.c \
	$(INC)/stdio.h \
	$(INC)/string.h \
	$(INC)/unistd.h \
	$(INC)/sys/types.h \
	$(INC)/sys/procset.h \
	$(INC)/sys/priocntl.h \
	priocntl.h

install: all
	$(INS) -f $(USRBIN) -u root -g root -m 4555 priocntl
	@if [ ! -d $(CLASSDIR)/FP ]; \
	then \
	mkdir -p  $(CLASSDIR)/FP; \
	fi;
	@if [ ! -d $(CLASSDIR)/FC ]; \
	then \
	mkdir -p  $(CLASSDIR)/FC; \
	fi;
	@if [ ! -d $(CLASSDIR)/TS ]; \
	then \
	mkdir -p  $(CLASSDIR)/TS; \
	fi;
	$(INS) -f $(CLASSDIR)/FP -u bin -g bin -m 555 FPpriocntl
	$(INS) -f $(CLASSDIR)/FC -u bin -g bin -m 555 FCpriocntl
	$(INS) -f $(CLASSDIR)/TS -u bin -g bin -m 555 TSpriocntl

clean:
	rm -f $(OBJECTS)

clobber: clean
	rm -f priocntl FPpriocntl TSpriocntl FCpriocntl

lintit:
	$(LINT) $(LINTFLAGS) $(SOURCES)
