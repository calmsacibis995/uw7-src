#	Copyright (c) 1987 SCO
#	  All Rights Reserved

#ident	"@(#)specfs.cmds:common/cmd/fs.d/specfs/specfs.mk	1.1"

include $(CMDRULES)

INSDIR = $(ETC)/fs/specfs
OWN = bin
GRP = bin

INCSYS = $(INC)
FRC =

all:	mount 

mount: mount.o
	$(CC) -o $@ $@.o $(LDFLAGS) $(LDLIBS) $(ROOTLIBS)

mount.o:	mount.c\
	$(INC)/stdio.h\
	$(INC)/signal.h\
	$(INC)/unistd.h\
	$(INC)/errno.h\
	$(INCSYS)/sys/mnttab.h\
	$(INCSYS)/sys/mount.h\
	$(INCSYS)/sys/types.h\
	$(FRC)

install: all
	[ -d $(INSDIR) ] || mkdir -p $(INSDIR)
	[ -d $(USRLIB)/fs/specfs ] || mkdir -p $(USRLIB)/fs/specfs
	$(INS) -f $(INSDIR) -m 0555 -u $(OWN) -g $(GRP) mount
	$(INS) -f $(USRLIB)/fs/specfs -m 0555 -u $(OWN) -g $(GRP) mount

clean:
	rm -f *.o

clobber:	clean
	rm -f mount
FRC:
