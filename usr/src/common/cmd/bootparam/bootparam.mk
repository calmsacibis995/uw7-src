#	copyright	"%c%"

#ident	"@(#)prtconf:common/cmd/bootparam/bootparam.mk	1.1"
#ident	"$Header$"

include $(CMDRULES)

INSDIR = $(USRBIN)
LDLIBS = $(LIBELF)
LINTFLAGS = -x
LOCALDEF = -D_KMEMUSER

all: bootparam

bootparam: bootparam.o
	$(CC) -o $@ $@.o $(LDFLAGS) $(LDLIBS) $(SHLIBS)

bootparam.o: bootparam.c \
	$(INC)/stdio.h \
	$(INC)/stdlib.h \
	$(INC)/unistd.h $(INC)/sys/unistd.h \
	$(INC)/fcntl.h $(INC)/sys/fcntl.h \
	$(INC)/sys/ksym.h \
	$(INC)/sys/elf.h

install: all
	$(INS) -f $(INSDIR) -m 02555 -u bin -g sys bootparam

clean:
	rm -f *.o

clobber: clean
	rm -f bootparam

lintit:
	lint $(LINTFLAGS) bootparam.c
