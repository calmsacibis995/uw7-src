#	copyright	"%c%"

#ident	"@(#)fsba:common/cmd/fsba/fsba.mk	1.5.5.2"
#ident "$Header$"

include $(CMDRULES)

INSDIR = $(USRSBIN)
OWN = bin
GRP = bin

OBJS = fsba.o bsize.o

all: fsba

fsba:	$(OBJS)
	$(CC) $(LDFLAGS) -o $@ $(OBJS) $(LDLIBS) $(SHLIBS)

fsba.o:	fsba.c \
	$(INC)/stdio.h \
	$(INC)/sys/param.h \
	$(INC)/sys/types.h \
	$(INC)/sys/fs/s5ino.h \
	$(INC)/sys/fs/s5param.h \
	$(INC)/sys/stat.h \
	$(INC)/sys/fs/s5filsys.h \
	$(INC)/sys/fs/s5dir.h \
	$(INC)/fcntl.h \
	fsba.h

bsize.o: bsize.c \
	$(INC)/sys/types.h \
	$(INC)/sys/fs/s5ino.h \
	$(INC)/sys/fs/s5param.h \
	$(INC)/sys/stat.h \
	$(INC)/sys/fs/s5filsys.h \
	$(INC)/sys/fs/s5dir.h \
	fsba.h

install: all
	-rm -f $(ETC)/fsba
	$(INS) -f $(INSDIR) -m 0555 -u $(OWN) -g $(GRP) fsba
	-$(SYMLINK) /usr/sbin/fsba $(ETC)/fsba

clean:
	rm -f *.o

clobber: clean
	rm -f fsba
