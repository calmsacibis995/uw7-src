#	Copyright (c) 1991, 1992  Intel Corporation
#	All Rights Reserved
#
#	INTEL CORPORATION CONFIDENTIAL INFORMATION
#
#	This software is supplied to USL under the terms of a license 
#	agreement with Intel Corporation and may not be copied nor 
#	disclosed except in accordance with the terms of that agreement.

#ident	"@(#)cdfs.cmds:common/cmd/fs.d/cdfs/mount/mount.mk	1.5.1.2"
#ident	"$Header$"


include		$(CMDRULES)

INSDIR1		= $(USRLIB)/fs/cdfs
INSDIR2		= $(ETC)/fs/cdfs
OWN			= bin
GRP			= bin

# Uncomment if mount is converted to use library functions instead of utils.
# LDLIBS		= -lcdfs -lgen
LDLIBS		= -lgen
LOCALDEF        = -D_KMEMUSER


all:	mount

mount:	mount.o
	$(CC) $(LDFLAGS) -o $@ $@.o $(LDLIBS) $(NOSHLIBS)

install:	all
	[ -d $(INSDIR1) ] || mkdir -p $(INSDIR1)
	[ -d $(INSDIR2) ] || mkdir -p $(INSDIR2)
	$(INS) -f $(INSDIR1) -m 0555 -u $(OWN) -g $(GRP) mount
	$(INS) -f $(INSDIR2) -m 0555 -u $(OWN) -g $(GRP) mount

headinstall:

lintit:
	$(LINT) $(LINTFLAGS) $(DEFLIST) mount.c

clean:
	-rm -f mount.o

clobber:	clean
	rm -f mount

mount.o:	mount.c \
	mount_local.h \
	$(INC)/stdio.h \
	$(INC)/unistd.h \
	$(INC)/sys/types.h \
	$(INC)/sys/errno.h \
	$(INC)/sys/mount.h \
	$(INC)/sys/mnttab.h \
	$(INC)/sys/signal.h \
	$(INC)/sys/statvfs.h \
	$(INC)/sys/fs/cdfs_fs.h \
	$(INC)/sys/fs/cdfs_inode.h \
	$(INC)/sys/fs/cdfs_ioctl.h

