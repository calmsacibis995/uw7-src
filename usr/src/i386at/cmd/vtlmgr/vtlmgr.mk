#ident	"@(#)vtlmgr.mk	1.5"

include	$(CMDRULES)

MAINS	= vtlmgr newvt vtgetty vtautofocus
OBJECTS	= vtlmgr.o newvt.o vtgetty.o vtautofocus.o
SOURCES	= vtlmgr.c newvt.c vtgetty.c vtautofocus.c

LIMITED = -DLIMITED
CONS 	= -DCONSOLE='"/dev/console"' -DSECURITY $(LIMITED)
LDLIBS	= -lcmd -lgen -lcrypt_i

all: $(MAINS)

install: all
	$(INS) -f $(USRBIN) -m 2555  -u bin  -g tty vtlmgr
	$(INS) -f $(USRBIN) -m 555   -u bin  -g bin newvt
	$(INS) -f $(USRSBIN)   -m 544   -u root -g bin vtgetty
	$(INS) -f $(USRBIN) -m 555   -u bin  -g bin vtautofocus

vtautofocus: vtautofocus.o
	$(CC) -o vtautofocus vtautofocus.o $(LDFLAGS)

vtautofocus: vtautofocus.o
	$(INC)/stdio.h \
	$(INC)/unistd.h \
	$(INC)/fcntl.h \
	$(INC)/sys/types.h \
	$(INC)/sys/kd.h \
	$(INC)/sys/stat.h \

vtlmgr:	vtlmgr.o
	$(CC) -o vtlmgr vtlmgr.o $(LDFLAGS)


vtlmgr.o: $(INC)/fcntl.h \
	$(INC)/signal.h \
	$(INC)/stdio.h \
	$(INC)/string.h \
	$(INC)/sys/at_ansi.h \
	$(INC)/sys/kd.h \
	$(INC)/sys/termio.h \
	$(INC)/sys/types.h \
	$(INC)/sys/vt.h

newvt: newvt.o
	$(CC) -o newvt newvt.o $(LDFLAGS) 

newvt.o: $(INC)/fcntl.h \
	$(INC)/signal.h \
	$(INC)/stdio.h \
	$(INC)/string.h \
	$(INC)/ctype.h \
	$(INC)/sys/at_ansi.h \
	$(INC)/sys/kd.h \
	$(INC)/sys/termio.h \
	$(INC)/sys/types.h \
	$(INC)/sys/vt.h

vtgetty: vtgetty.o
	$(CC) -o vtgetty vtgetty.o $(LDFLAGS)

vtgetty.o: $(INC)/fcntl.h \
	$(INC)/signal.h \
	$(INC)/stdio.h \
	$(INC)/string.h \
	$(INC)/sys/at_ansi.h \
	$(INC)/sys/kd.h \
	$(INC)/sys/termio.h \
	$(INC)/sys/types.h \
	$(INC)/sys/vt.h

clean:
	rm -f $(OBJECTS)

clobber: clean
	rm -f $(MAINS)
