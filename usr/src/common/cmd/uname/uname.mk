#	copyright	"%c%"

#ident	"@(#)uname:common/cmd/uname/uname.mk	1.4.11.3"
#ident	"$Header$"

include $(CMDRULES)

#		PROPRIETARY NOTICE (Combined)
#
#This source code is unpublished proprietary information
#constituting, or derived under license from AT&T's UNIX(r) System V.
#In addition, portions of such source code were derived from Berkeley
#4.3 BSD under license from the Regents of the University of
#California.
#
#
#
#		Copyright Notice 
#
#Notice of copyright on this source code product does not indicate 
#publication.
#
#	(c) 1986,1987,1988,1989  Sun Microsystems, Inc
#	(c) 1983,1984,1985,1986,1987,1988,1989  AT&T.
#	          All rights reserved.

#	Makefile for uname 

OWN = bin
GRP = bin
MAINS = uname
OBJECTS = uname.o setuname.o

all: $(MAINS)

uname: $(OBJECTS)
	$(CC) $(CFLAGS) -o $@ $(OBJECTS) $(LDFLAGS) $(LDLIBS) $(SHLIBS)

uname.o: uname.c \
	$(INC)/stdio.h \
	$(INC)/stdlib.h \
	$(INC)/unistd.h \
	$(INC)/locale.h \
	$(INC)/pfmt.h \
	$(INC)/errno.h \
	$(INC)/sys/errno.h \
	$(INC)/sys/syscall.h \
	$(INC)/sys/systeminfo.h \
	$(INC)/sys/utsname.h \
	$(INC)/string.h

setuname.o: setuname.c \
	$(INC)/stdio.h \
	$(INC)/stdlib.h \
	$(INC)/unistd.h \
	$(INC)/locale.h \
	$(INC)/pfmt.h \
	$(INC)/ctype.h \
	$(INC)/errno.h \
	$(INC)/sys/errno.h \
	$(INC)/string.h \
	$(INC)/signal.h \
	$(INC)/sys/utsname.h \
	$(INC)/sys/systeminfo.h \
	$(INC)/sys/types.h \
	$(INC)/sys/stat.h

install: all
	$(INS) -f $(SBIN) -m 0555 -u $(OWN) -g $(GRP) uname
	$(INS) -f $(USRBIN) -m 0555 -u $(OWN) -g $(GRP) uname

clean:
	rm -f $(OBJECTS)

clobber: clean
	rm -f $(MAINS)

lintit:
	$(LINT) $(LINTFLAGS) $(INCLIST) uname.c setuname.c

#	These targets are useful but optional

partslist:
	@echo uname.mk uname.c setuname.c $(LOCALINCS) | tr ' ' '\012' | sort

productdir:
	@echo $(USRBIN) | tr ' ' '\012' | sort

product:
	@echo uname | tr ' ' '\012' | \
	sed 's;^;$(USRBIN)/;'

srcaudit:
	@fileaudit uname.mk $(LOCALINCS) uname.c -o uname.o uname
