#	Copyright (c) 1984, 1986, 1987, 1988, 1989 AT&T
#	  All Rights Reserved

#	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF AT&T
#	The copyright notice above does not evidence any
#	actual or intended publication of such source code.

#ident	"@(#)bfs.cmds:common/cmd/fs.d/bfs/bfs.mk	1.9.5.2"
#ident "$Header$"

include $(CMDRULES)

DIR = $(ETC)/fs/bfs $(USRLIB)/fs/bfs
FRC =

CMDS =\
	fsck \
	mount \
	mkfs

all: $(CMDS)

fsck:
	$(MAKE) -f fsck.mk $(MAKEARGS)

mount:
	$(MAKE) -f mount.mk $(MAKEARGS)

mkfs:
	$(MAKE) -f mkfs.mk $(MAKEARGS)

clean:
	rm -f *.o

install: $(CMDS) $(DIR)
	$(MAKE) -f fsck.mk install $(MAKEARGS)
	$(MAKE) -f mount.mk install $(MAKEARGS)
	$(MAKE) -f mkfs.mk install $(MAKEARGS)

$(DIR):
	mkdir -p $@

clobber: clean
	$(MAKE) -f fsck.mk  $@ $(MAKEARGS)
	$(MAKE) -f mount.mk $@ $(MAKEARGS)
	$(MAKE) -f mkfs.mk  $@ $(MAKEARGS)
