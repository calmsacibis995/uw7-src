#	copyright	"%c%"

#ident	"@(#)whodo:whodo.mk	1.9.11.3"
#ident "$Header$"

include $(CMDRULES)

OWN = bin
GRP = bin

LDLIBS = -lcmd

all: whodo

whodo: whodo.o
	$(CC) -o whodo whodo.o $(LDFLAGS) $(LDLIBS) $(SHLIBS) 

whodo.o: whodo.c \
	$(INC)/stdio.h \
	$(INC)/stdlib.h \
	$(INC)/fcntl.h \
	$(INC)/time.h \
	$(INC)/sys/errno.h \
	$(INC)/sys/types.h \
	$(INC)/sys/param.h \
	$(INC)/sys/time.h \
	$(INC)/priv.h \
	$(INC)/mac.h \
	$(INC)/deflt.h \
	$(INC)/utmp.h \
	$(INC)/sys/utsname.h \
	$(INC)/sys/stat.h \
	$(INC)/dirent.h \
	$(INC)/sys/procfs.h \
	$(INC)/sys/proc.h

install: all
	-rm -f $(ETC)/whodo
	$(INS) -f $(USRSBIN) -m 0555 -u $(OWN) -g $(GRP) whodo
	-$(SYMLINK) /usr/sbin/whodo $(ETC)/whodo

clean:
	rm -f whodo.o

clobber: clean
	rm -f whodo

lintit:
	$(LINT) $(LINTFLAGS) whodo.d
