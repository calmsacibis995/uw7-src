#	copyright	"%c%"

#ident	"@(#)ksh:sh/sh.mk	1.7.1.1"

include $(CMDRULES)

# makefile for ksh 

KSHINC   = ../include
LOCALINC = -I$(KSHINC)
LOCALDEF = -DKSHELL -DMULTIBYTE -D_locale_ -DCHILD_MAX=1024
LDLIBS   = ../shlib/libsh.a -lw

SOURCES = args.c arith.c builtin.c cmd.c ctype.c defs.c echo.c \
	edit.c emacs.c error.c expand.c fault.c history.c \
	io.c jobs.c macro.c main.c msg.c name.c outmsg.c print.c \
	service.c stak.c string.c test.c vfork.c vi.c word.c xec.c

OBJECTS = $(SOURCES:.c=.o)

HEADER	= $(KSHINC)/defs.h \
	$(KSHINC)/sh_config.h \
	$(INC)/sys/types.h \
	$(INC)/setjmp.h \
	$(INC)/signal.h $(INC)/sys/signal.h \
	$(INC)/unistd.h $(INC)/sys/unistd.h \
	$(INC)/sys/times.h \
	$(KSHINC)/name.h \
	$(KSHINC)/flags.h \
	$(KSHINC)/shnodes.h \
	$(KSHINC)/stak.h \
	$(KSHINC)/io.h \
	$(INC)/sys/stat.h \
	$(INC)/fcntl.h $(INC)/sys/fcntl.h \
	$(INC)/sys/file.h \
	$(KSHINC)/brkincr.h \
	$(KSHINC)/shtype.h \
	$(KSHINC)/outmsg.h

TERMHEAD = $(KSHINC)/terminal.h \
	$(INC)/termio.h $(INC)/sys/termio.h \
	$(INC)/termios.h $(INC)/sys/termios.h \
	$(INC)/sys/time.h \
	$(INC)/sys/filio.h


all:  $(OBJECTS)
	$(CC) -o ksh $(OBJECTS) $(LDFLAGS) -Kosrcrt $(LDLIBS) $(SHLIBS)

apollo.o: apollo.c \
	$(HEADER) \
	$(INC)/errno.h $(INC)/sys/errno.h

args.o: args.c \
	$(HEADER) \
	$(KSHINC)/jobs.h \
	$(TERMHEAD) \
	$(KSHINC)/sym.h \
	$(KSHINC)/builtins.h

arith.o: arith.c \
	$(HEADER) \
	$(KSHINC)/streval.h

builtin.o: builtin.c \
	$(INC)/errno.h $(INC)/sys/errno.h \
	$(HEADER) \
	$(KSHINC)/history.h \
	$(KSHINC)/builtins.h \
	$(KSHINC)/jobs.h \
	$(TERMHEAD) \
	$(KSHINC)/sym.h \
	$(INC)/sys/resource.h \
	$(INC)/poll.h

cmd.o: cmd.c \
	$(HEADER) \
	$(KSHINC)/sym.h \
	$(KSHINC)/history.h \
	$(KSHINC)/builtins.h \
	$(KSHINC)/test.h

ctype.o: ctype.c \
	$(KSHINC)/sh_config.h \
	$(INC)/sys/types.h \
	$(KSHINC)/shtype.h

defs.o: defs.c \
	$(HEADER) \
	$(KSHINC)/jobs.h \
	$(TERMHEAD) \
	$(KSHINC)/sym.h \
	$(KSHINC)/history.h \
	$(KSHINC)/edit.h \
	$(INC)/ctype.h \
	$(KSHINC)/national.h \
	$(KSHINC)/timeout.h

echo.o: echo.c \
	$(HEADER)

edit.o: edit.c \
	$(INC)/errno.h $(INC)/sys/errno.h \
	$(HEADER) \
	$(TERMHEAD) \
	$(KSHINC)/builtins.h \
	$(KSHINC)/sym.h \
	$(KSHINC)/history.h \
	$(KSHINC)/edit.h \
	$(INC)/ctype.h \
	$(KSHINC)/national.h \
	$(INC)/sys/stream.h \
	$(INC)/sys/ptem.h \
	$(INC)/sys/jioctl.h

editlib.o: editlib.c \
	$(KSHINC)/io.h \
	$(KSHINC)/sh_config.h \
	$(INC)/sys/types.h \
	$(INC)/sys/stat.h \
	$(INC)/unistd.h $(INC)/sys/unistd.h \
	$(INC)/fcntl.h $(INC)/sys/fcntl.h \
	$(INC)/sys/file.h \
	$(TERMHEAD) \
	$(KSHINC)/history.h \
	$(INC)/setjmp.h \
	$(KSHINC)/edit.h \
	$(INC)/signal.h $(INC)/sys/signal.h \
	$(INC)/ctype.h \
	$(KSHINC)/national.h \
	$(INC)/sys/ioctl.h

emacs.o: emacs.c \
	$(HEADER) \
	$(KSHINC)/history.h \
	$(INC)/ctype.h \
	$(KSHINC)/national.h \
	$(KSHINC)/edit.h

error.o: error.c \
	$(HEADER) \
	$(KSHINC)/jobs.h \
	$(TERMHEAD) \
	$(KSHINC)/history.h

expand.o: expand.c \
	$(HEADER) \
	$(INC)/dirent.h $(INC)/sys/dirent.h \
	$(INC)/sys/fs/s5dir.h

fault.o: fault.c \
	$(HEADER) \
	$(KSHINC)/jobs.h \
	$(TERMHEAD) \
	$(KSHINC)/sym.h \
	$(KSHINC)/timeout.h

history.o: history.c \
	$(HEADER) \
	$(KSHINC)/builtins.h \
	$(INC)/ctype.h \
	$(KSHINC)/history.h \
	$(KSHINC)/national.h

io.o: io.c \
	$(INC)/errno.h $(INC)/sys/errno.h \
	$(HEADER) \
	$(KSHINC)/sym.h \
	$(KSHINC)/history.h \
	$(INC)/sys/socket.h \
	$(INC)/netinet/in.h \
	$(INC)/sys/ioctl.h

jobs.o: jobs.c \
	$(INC)/errno.h $(INC)/sys/errno.h \
	$(HEADER) \
	$(KSHINC)/jobs.h \
	$(TERMHEAD) \
	$(KSHINC)/history.h \
	$(INC)/sys/wait.h \
	$(INC)/wait.h

macro.o: macro.c \
	$(HEADER) \
	$(KSHINC)/sym.h \
	$(KSHINC)/builtins.h \
	$(KSHINC)/national.h

main.o: main.c \
	$(HEADER) \
	$(KSHINC)/jobs.h \
	$(TERMHEAD) \
	$(KSHINC)/sym.h \
	$(KSHINC)/history.h \
	$(KSHINC)/timeout.h \
	$(KSHINC)/builtins.h \
	$(INC)/sys/ioctl.h

msg.o: msg.c \
	$(INC)/errno.h $(INC)/sys/errno.h \
	$(HEADER) \
	$(KSHINC)/sym.h \
	$(KSHINC)/builtins.h \
	$(KSHINC)/test.h \
	$(KSHINC)/timeout.h \
	$(KSHINC)/history.h \
	$(KSHINC)/national.h \
	$(INC)/sys/time.h \
	$(INC)/sys/resource.h

name.o: name.c \
	$(HEADER) \
	$(KSHINC)/sym.h \
	$(KSHINC)/builtins.h \
	$(KSHINC)/history.h \
	$(KSHINC)/timeout.h \
	$(INC)/locale.h \
	$(KSHINC)/national.h

outmsg.o: outmsg.c \
	$(HEADER) \
	$(INC)/pfmt.h \
	$(INC)/getwidth.h \
	$(INC)/sys/euc.h \
	$(INC)/errno.h

print.o: print.c \
	$(HEADER) \
	$(KSHINC)/builtins.h

service.o: service.c \
	$(INC)/errno.h $(INC)/sys/errno.h \
	$(HEADER) \
	$(KSHINC)/jobs.h \
	$(TERMHEAD) \
	$(KSHINC)/sym.h \
	$(KSHINC)/builtins.h \
	$(KSHINC)/history.h \
	$(INC)/sys/acct.h

stak.o: stak.c \
	$(KSHINC)/stak.h

string.o: string.c \
	$(HEADER) \
	$(KSHINC)/sym.h \
	$(KSHINC)/national.h

test.o: test.c \
	$(HEADER) \
	$(KSHINC)/test.h \
	$(KSHINC)/sym.h

vfork.o: vfork.c \
	$(HEADER) \
	$(KSHINC)/jobs.h \
	$(TERMHEAD) \
	$(KSHINC)/sym.h \
	$(KSHINC)/builtins.h

vi.o: vi.c \
	$(INC)/errno.h $(INC)/sys/errno.h \
	$(HEADER) \
	$(KSHINC)/history.h \
	$(INC)/setjmp.h \
	$(TERMHEAD)

word.o: word.c \
	$(HEADER) \
	$(KSHINC)/sym.h \
	$(KSHINC)/builtins.h \
	$(KSHINC)/test.h

xec.o: xec.c \
	$(INC)/errno.h $(INC)/sys/errno.h \
	$(HEADER) \
	$(KSHINC)/jobs.h \
	$(TERMHEAD) \
	$(KSHINC)/sym.h \
	$(KSHINC)/test.h \
	$(KSHINC)/builtins.h \
	$(INC)/sys/timeb.h

clean :
	rm -f $(OBJECTS)

clobber : clean
	rm -f ksh

lintit:
	$(LINT) $(LINTFLAGS) $(SOURCES)
