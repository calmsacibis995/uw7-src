#	Copyright (c) 1991, 1992  Intel Corporation
#	All Rights Reserved

#	INTEL CORPORATION CONFIDENTIAL INFORMATION

#	This software is supplied to USL under the terms of a license
#	agreement with Intel Corporation and may not be copied nor
#	disclosed except in accordance with the terms of that agreement.

#ident	"@(#)libcdfs:i386/lib/libhcdfs/libhcdfs.mk	1.11.1.3"
#ident	"$Header$"

#
# Makefile for libcdfs.
#

include $(LIBRULES)

MAKEFILE=libhcdfs.mk

OWN			= bin
GRP			= bin

LFLAGS		= -G -dy
HFLAGS		= -h /usr/lib/libcdfs.so
LOCALDEF	= $(PICFLAG) -D_KMEMUSER
LDLIBS		= -lgen

LIBRARY		= libcdfs.a
DOTSO		= libcdfs.so
LINTLIB		= llib-lcdfs.ln

OBJECTS		= cdfs_lib.o
SOURCES		= cdfs_lib.c


.MUTEX:	archive shared

all:	archive shared

archive:
	$(MAKE) -f $(MAKEFILE) clean $(LIBRARY) PICFLAG=''

shared:
	$(MAKE) -f $(MAKEFILE) clean $(DOTSO) PICFLAG='$(PICFLAG)'

$(LIBRARY):	$(OBJECTS)
	$(AR) $(ARFLAGS) $(LIBRARY) $(OBJECTS)

$(DOTSO):	$(OBJECTS)
	$(CC) $(LFLAGS) $(HFLAGS) -o $(DOTSO) $(OBJECTS) $(LDLIBS)

install: all
	$(INS) -f $(USRLIB) -m 0644 -u $(OWN) -g $(GRP) $(LIBRARY)
	$(INS) -f $(USRLIB) -m 0755 -u $(OWN) -g $(GRP) $(DOTSO)

headinstall:

lintit:	
	-$(LINT) $(LINTFLAGS) $(DEFLIST) *.c
	
clean:
	-rm -f $(OBJECTS)

clobber:	clean
	-rm -f $(LIBRARY) $(DOTSO)


#
# Header dependencies.
#

cdfs_lib.o:	cdfs_lib.c \
	cdfs_libdef.h \
	$(INC)/errno.h \
	$(INC)/fcntl.h \
	$(INC)/signal.h \
	$(INC)/stdio.h \
	$(INC)/stdlib.h \
	$(INC)/string.h \
	$(INC)/time.h \
	$(INC)/unistd.h \
	$(INC)/sys/cdrom.h \
	$(INC)/sys/fcntl.h \
	$(INC)/sys/ioctl.h \
	$(INC)/sys/mkdev.h \
	$(INC)/sys/mnttab.h \
	$(INC)/sys/param.h \
	$(INC)/sys/pathname.h \
	$(INC)/sys/proc.h \
	$(INC)/sys/stat.h \
	$(INC)/sys/statvfs.h \
	$(INC)/sys/sysmacros.h \
	$(INC)/sys/types.h \
	$(INC)/sys/vfs.h \
	$(INC)/sys/vnode.h \
	$(INC)/sys/fs/cdfs_fs.h \
	$(INC)/sys/fs/cdfs_inode.h \
	$(INC)/sys/fs/cdfs_ioctl.h \
	$(INC)/sys/fs/cdfs_susp.h \
	$(INC)/sys/fs/iso9660.h
