#	copyright	"%c%"

#ident	"@(#)format:i386/cmd/format/format.mk	1.4.6.3"
#ident	"$Header$"

include $(CMDRULES)

INSDIR = $(USRSBIN)
LOCALDEF= -D$(MACH)

FRC =

MSGS =	format.str

all:	format

install: all $(MSGS)
	-rm -f $(ETC)/format
	$(INS) -f $(INSDIR) format
	$(SYMLINK) /usr/sbin/format $(ETC)/format
	-[ -d $(USRLIB)/locale/C/MSGFILES ] || \
		mkdir -p $(USRLIB)/locale/C/MSGFILES
	$(INS) -f $(USRLIB)/locale/C/MSGFILES -m 644 format.str

format: format.o devcheck.o
	$(CC) -o $@ format.o devcheck.o $(LDFLAGS) $(LDLIBS) $(SHLIBS)

devcheck.o: devcheck.c \
	$(INC)/stdio.h

format.o: format.c \
	$(INC)/sys/types.h \
	$(INC)/sys/mkdev.h \
	$(INC)/sys/stat.h \
	$(INC)/sys/param.h \
	$(INC)/sys/buf.h \
	$(INC)/sys/iobuf.h \
	$(INC)/sys/vtoc.h \
	$(INC)/stdio.h \
	$(INC)/errno.h

clean:
	-rm -f *.o

clobber: clean
	-rm -f format

FRC:

#
# Header dependencies
#

format: format.c \
	$(INC)/sys/types.h \
	$(INC)/stdio.h \
	$(INC)/sys/param.h \
	$(INC)/sys/buf.h \
	$(INC)/sys/iobuf.h \
	$(INC)/errno.h \
	$(INC)/sys/vtoc.h \
	$(FRC)
