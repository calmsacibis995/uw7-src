#	copyright	"%c%"

#ident	"@(#)bfs.cmds:common/cmd/fs.d/bfs/mount.mk	1.8.9.4"
#ident "$Header$"

include $(CMDRULES)

OWN = bin
GRP = bin

FRC =

FILES =\
	mount.o

all: mount

mount: $(FILES)
	$(CC) $(LDFLAGS) -o $@ $(FILES) $(LDLIBS) $(ROOTLIBS)
	$(CC) $(LDFLAGS) -o $@.dy $(FILES) $(LDLIBS) $(SHLIBS)

install: mount
	$(INS) -f $(ETC)/fs/bfs -m 0555 -u $(OWN) -g $(GRP) mount
	$(INS) -f $(USRLIB)/fs/bfs -m 0555 -u $(OWN) -g $(GRP) mount
	$(INS) -f $(ETC)/fs/bfs -m 0555 -u $(OWN) -g $(GRP) mount.dy
	$(INS) -f $(USRLIB)/fs/bfs -m 0555 -u $(OWN) -g $(GRP) mount.dy

clean:
	rm -f *.o

clobber: clean
	rm -f mount mount.dy

#
# Header dependencies
#

mount.o: mount.c \
	$(INC)/stdio.h \
	$(INC)/signal.h \
	$(INC)/unistd.h \
	$(INC)/errno.h \
	$(INC)/sys/mnttab.h \
	$(INC)/sys/mount.h \
	$(INC)/sys/types.h \
	$(FRC)
