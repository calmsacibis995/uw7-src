#	copyright	"%c%"

#ident	"@(#)gcore:i386/cmd/gcore/gcore.mk	1.2.7.1"
#ident  "$Header$"

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

#	Makefile for gcore

OWN = bin
GRP = bin

all: gcore

gcore: gcore.o
	$(CC) -o gcore gcore.o $(LDFLAGS) $(LDLIBS) $(SHLIBS)

gcore.o: gcore.c \
	$(INC)/ctype.h \
	$(INC)/errno.h $(INC)/sys/errno.h \
	$(INC)/fcntl.h $(INC)/sys/fcntl.h \
	$(INC)/limits.h \
	$(INC)/signal.h $(INC)/sys/signal.h \
	$(INC)/stdio.h \
	$(INC)/stdlib.h \
	$(INC)/string.h \
	$(INC)/unistd.h \
	$(INC)/sys/elf.h \
	$(INC)/sys/file.h \
	$(INC)/sys/fsid.h \
	$(INC)/sys/fstyp.h \
	$(INC)/sys/mman.h \
	$(INC)/sys/param.h \
	$(INC)/sys/procfs.h \
	$(INC)/sys/stat.h \
	$(INC)/sys/sysmacros.h \
	$(INC)/sys/types.h \
	$(INC)/sys/user.h \
	$(INC)/sys/utsname.h

install: all
	$(INS) -f $(USRBIN) -m 0555 -u $(OWN) -g $(GRP) gcore 

clean:
	rm -f gcore.o

clobber: clean
	rm -f gcore

lintit:
	$(LINT) $(LINTFLAGS) gcore.c

#	These targets are useful but optional

partslist:
	@echo gcore.mk gcore.c $(LOCALINCS) | tr ' ' '\012' | sort

productdir:
	@echo $(USRBIN) | tr ' ' '\012' | sort

product:
	@echo gcore | tr ' ' '\012' | \
	sed 's;^;$(USRBIN)/;'

srcaudit:
	@fileaudit gcore.mk $(LOCALINCS) gcore.c -o gcore.o gcore
