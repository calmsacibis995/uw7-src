#	Copyright (c) 1991, 1992  Intel Corporation
#	All Rights Reserved
#
#	INTEL CORPORATION CONFIDENTIAL INFORMATION
#
#	This software is supplied to USL under the terms of a license 
#	agreement with Intel Corporation and may not be copied nor 
#	disclosed except in accordance with the terms of that agreement.

#ident	"@(#)cdfs.cmds:common/cmd/fs.d/cdfs/cdmntsuppl/cdmntsuppl.mk	1.5.1.1"
#ident	"$Header$"

# Tabstops: 4



include		$(CMDRULES)

INSDIR		= $(USRLIB)/fs/cdfs
OWN			= bin
GRP			= bin

LDLIBS		= -lcdfs -lgen
LOCALDEF        = -D_KMEMUSER


all:	cdmntsuppl

cdmntsuppl:	cdmntsuppl.o
	$(CC) $(LDFLAGS) -o $@ $@.o $(LDLIBS)

install:	all
	[ -d $(INSDIR) ] || mkdir -p $(INSDIR)
	$(INS) -f $(INSDIR) -m 0555 -u $(OWN) -g $(GRP) cdmntsuppl

headinstall:

lintit:
	$(LINT) $(LINTFLAGS) $(DEFLIST) cdmntsuppl.c

clean:
	-rm -f *.o
	-rm -f *.ln

clobber:	clean
	rm -f cdmntsuppl

cdmntsuppl.o:	cdmntsuppl.c \
	cdmntsuppl.h \
	$(INC)/ctype.h \
	$(INC)/errno.h \
	$(INC)/grp.h \
	$(INC)/libgen.h \
	$(INC)/locale.h \
	$(INC)/pfmt.h \
	$(INC)/stdio.h \
	$(INC)/stdlib.h \
	$(INC)/string.h \
	$(INC)/sys/cdrom.h \
	$(INC)/sys/param.h \
	$(INC)/sys/stat.h \
	$(INC)/sys/types.h \
	$(INC)/sys/fs/cdfs_fs.h \
	$(INC)/sys/fs/cdfs_ioctl.h

