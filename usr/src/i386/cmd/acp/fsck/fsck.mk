#	Copyright (c) 1987, 1988 Microsoft Corporation
#	  All Rights Reserved

#	This Module contains Proprietary Information of Microsoft 
#	Corporation and should be treated as Confidential.

#ident	"@(#)acp:i386/cmd/acp/fsck/fsck.mk	1.2.1.5"
include	$(CMDRULES)

INSDIR = $(ETC)/fs/XENIX
FRC =
LOCALDEF = -D_KMEMUSER

all:	fsck

fsck: fsck1.o fsck2.o
	$(CC) -o fsck fsck1.o fsck2.o $(LDFLAGS)

fsck1.o:\
	fsck.h\
	$(INC)/stdio.h\
	$(INC)/ctype.h\
	$(INC)/signal.h\
	$(INC)/sys/types.h\
	$(INC)/sys/param.h\
	$(INC)/sys/fs/s5param.h\
	$(INC)/sys/fs/xxfilsys.h\
	$(INC)/sys/fs/s5dir.h\
	$(INC)/sys/fs/xxfblk.h\
	$(INC)/sys/ino.h\
	$(INC)/sys/inode.h\
	$(INC)/sys/fs/s5inode.h\
	$(INC)/sys/stat.h\
	fsck1.c\
	$(FRC)

fsck2.o:\
	$(INC)/sys/sysmacros.h\
	fsck.h\
	$(INC)/stdio.h\
	$(INC)/ctype.h\
	$(INC)/signal.h\
	$(INC)/sys/types.h\
	$(INC)/sys/param.h\
	$(INC)/sys/fs/s5param.h\
	$(INC)/sys/fs/xxfilsys.h\
	$(INC)/sys/fs/s5dir.h\
	$(INC)/sys/fs/xxfblk.h\
	$(INC)/sys/ino.h\
	$(INC)/sys/inode.h\
	$(INC)/sys/fs/s5inode.h\
	$(INC)/sys/stat.h\
	fsck2.c\
	$(FRC)

FRC :

install: fsck $(INSDIR)
	$(INS) -f $(INSDIR) -m 555 -u bin -g bin  fsck

$(INSDIR):
	-mkdir -p $@
	$(CH)chmod 755 $@
	$(CH)chgrp sys $@
	$(CH)chown root $@

clean:
	rm -f *.o

clobber : clean
	rm -f fsck
