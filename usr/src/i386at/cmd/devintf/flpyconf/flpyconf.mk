#ident	"@(#)flpyconf.mk	1.4"
#ident "$Header"

include $(CMDRULES)

INSDIR1 = $(USRSADM)/sysadm/bin
DIRS	= $(INSDIR1)


OWN = bin
GRP = bin

all: flpyconf

flpyconf: flpyconf.o
	$(CC) $(CFLAGS) $(LDFLAGS) $(DEFLIST) -dn -o $@ $@.o

flpyconf.o: flpyconf.c \
	$(INC)/errno.h \
	$(INC)/fcntl.h \
	$(INC)/stdio.h \
	$(INC)/sys/bootinfo.h \
	$(INC)/sys/cram.h \
	$(INC)/sys/errno.h \
	$(INC)/sys/fcntl.h \
	$(INC)/sys/select.h \
	$(INC)/sys/types.h

clean:
	rm -f *.o

clobber: clean
	rm -f flpyconf

lintit:

size strip: all

$(DIRS):
	-mkdir -p $@

install: all $(DIRS)
	$(INS) -u $(OWN) -g $(GRP) -m 555 -f $(INSDIR1) flpyconf
