#	Copyright (c) 1987, 1988 Microsoft Corporation
#	  All Rights Reserved

#	This Module contains Proprietary Information of Microsoft 
#	Corporation and should be treated as Confidential.

#ident	"@(#)acp:i386/cmd/acp/mount/mount.mk	1.2.1.3"

include	$(CMDRULES)

INSDIR	= $(ETC)/fs/XENIX

all:	mount

mount.o:	mount.c\
	$(INC)/sys/signal.h\
	$(INC)/unistd.h\
	$(INC)/sys/errno.h\
	$(INC)/sys/mnttab.h\
	$(INC)/sys/mount.h\
	$(INC)/sys/types.h\
	$(INC)/sys/statvfs.h

mount:	mount.o
	$(CC) -o mount mount.o $(LDFLAGS)

install: mount $(INSDIR)
	$(INS) -f $(INSDIR) -m 555 -u bin -g bin  mount

$(INSDIR):
	-mkdir -p $@
	$(CH)chmod 755 $@
	$(CH)chgrp sys $@
	$(CH)chown root $@

clean:
	rm -f mount.o

clobber : clean
	rm -f mount
