#	Copyright (c) 1991, 1992  Intel Corporation
#	All Rights Reserved
#
#	INTEL CORPORATION CONFIDENTIAL INFORMATION
#
#	This software is supplied to USL under the terms of a license 
#	agreement with Intel Corporation and may not be copied nor 
#	disclosed except in accordance with the terms of that agreement.

#ident	"@(#)cdfs.cmds:common/cmd/fs.d/cdfs/fstyp/fstyp.mk	1.6.1.2"
#ident	"$Header$"


include		$(CMDRULES)

INSDIR1		= $(USRLIB)/fs/cdfs
INSDIR2		= $(ETC)/fs/cdfs
OWN			= bin
GRP			= bin

LDLIBS		= -lcdfs -lgen
LOCALDEF        = -D_KMEMUSER


all:	fstyp

fstyp:	fstyp.o
	$(CC) $(LDFLAGS) -o $@ $@.o $(LDLIBS) $(NOSHLIBS)

install:	all
	[ -d $(INSDIR1) ] || mkdir -p $(INSDIR1)
	[ -d $(INSDIR2) ] || mkdir -p $(INSDIR2)
	$(INS) -f $(INSDIR1) -m 0555 -u $(OWN) -g $(GRP) fstyp
	$(INS) -f $(INSDIR2) -m 0555 -u $(OWN) -g $(GRP) fstyp

headinstall:

lintit:
	$(LINT) $(LINTFLAGS) $(DEFLIST) fstyp.c

clean:
	rm -f fstyp.o

clobber:	clean
	rm -f fstyp


fstyp.o: fstyp.c \
	$(INC)/fcntl.h \
	$(INC)/stdlib.h \
	$(INC)/stdio.h \
	$(INC)/unistd.h \
	$(INC)/sys/types.h \
	$(INC)/sys/param.h \
	$(INC)/sys/fs/cdfs_fs.h \
	$(INC)/sys/fs/cdfs_inode.h \
	$(INC)/sys/fs/cdfs_ioctl.h

