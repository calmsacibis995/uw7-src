#	Copyright (c) 1984, 1986, 1987, 1988, 1989 AT&T
#	  All Rights Reserved

#	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF AT&T
#	The copyright notice above does not evidence any
#	actual or intended publication of such source code.

#ident	"@(#)bfs.cmds:common/cmd/fs.d/bfs/labelit.mk	1.1.1.2"
#ident "$Header$"

include $(CMDRULES)

OWN = bin
GRP = bin

INCSYS = $(INC)
FRC =

FILES =\
	labelit.o

all: labelit

labelit: $(FILES)
	$(CC) -o $@ $(FILES) $(LDFLAGS) $(LDLIBS) $(ROOTLIBS)

install: labelit
	$(INS) -f $(ETC)/fs/bfs -m 0555 -u $(OWN) -g $(GRP) labelit
	$(INS) -f $(USRLIB)/fs/bfs -m 0555 -u $(OWN) -g $(GRP) labelit

clean:
	rm -f *.o

clobber: clean
	rm -f labelit labelit.dy *.o
#
# Header dependencies
#

labelit.o: labelit.c \
	$(INC)/stdio.h \
	$(INCSYS)/sys/types.h \
	$(INCSYS)/sys/vnode.h \
	$(INCSYS)/sys/fs/bfs.h \
	$(INCSYS)/sys/vtoc.h \
	$(INCSYS)/sys/stat.h \
	$(INCSYS)/sys/sysmacros.h \
	$(INCSYS)/sys/fcntl.h \
	$(FRC)
