#	copyright	"%c%"

#ident	"@(#)sh:common/cmd/sh/sh.mk	1.18.25.1"
#ident "$Header$"

include $(CMDRULES)

#	Makefile for sh

OWN = bin
GRP = root

LOCALDEF = -DACCT
LDLIBS = -lcmd -lgen -lw

MAINS = sh sh.dy
SOURCES = args.c blok.c bltin.c cmd.c ctype.c defs.c echo.c error.c expand.c \
	fault.c func.c hash.c hashserv.c io.c jobs.c macro.c main.c msg.c \
	name.c print.c prv.c pwd.c service.c setbrk.c stak.c string.c test.c \
	ulimit.c umask.c word.c xec.c

OBJECTS = $(SOURCES:.c=.o)

all: $(MAINS)

sh: $(OBJECTS)
	$(CC) -o $@ $(OBJECTS) $(LDFLAGS) -Kosrcrt $(LDLIBS) $(ROOTLIBS)

sh.dy: $(OBJECTS)
	$(CC) -o $@ $(OBJECTS) $(LDFLAGS) -Kosrcrt $(LDLIBS) $(SHLIBS)

LOCALHEADS = defs.h \
	mac.h \
	mode.h \
	name.h \
	$(INC)/signal.h $(INC)/sys/signal.h \
	$(INC)/sys/types.h \
	stak.h \
	$(INC)/setjmp.h \
	brkincr.h \
	ctype.h \
	$(INC)/ctype.h \
	$(INC)/locale.h \
	$(INC)/stdlib.h \
	$(INC)/limits.h

args.o: args.c \
	$(LOCALHEADS)

blok.o: blok.c \
	$(INC)/memory.h \
	$(LOCALHEADS)

bltin.o: bltin.c \
	$(LOCALHEADS) \
	$(INC)/errno.h $(INC)/sys/errno.h \
	sym.h \
	hash.h \
	$(INC)/sys/types.h \
	$(INC)/sys/times.h \
	$(INC)/sys/time.h \
	$(INC)/mac.h \
	$(INC)/priv.h \
	$(INC)/pfmt.h \
	$(INC)/string.h

cmd.o: cmd.c \
	$(INC)/pfmt.h \
	$(LOCALHEADS) \
	sym.h \
	hash.h

ctype.o: ctype.c \
	$(LOCALHEADS)

defs.o: defs.c \
	$(INC)/setjmp.h \
	mode.h \
	name.h \
	$(INC)/sys/param.h

echo.o: echo.c \
	$(LOCALHEADS)

error.o: error.c \
	$(INC)/pfmt.h \
	$(INC)/string.h \
	$(LOCALHEADS)

expand.o: expand.c \
	$(LOCALHEADS) \
	$(INC)/sys/types.h \
	$(INC)/sys/stat.h \
	$(INC)/dirent.h

fault.o: fault.c \
	$(INC)/pfmt.h \
	$(INC)/unistd.h \
	$(LOCALHEADS) \
	$(INC)/sys/procset.h

func.o: func.c \
	$(LOCALHEADS)

hash.o: hash.c \
	hash.h \
	$(LOCALHEADS)

hashserv.o: hashserv.c \
	hash.h \
	$(LOCALHEADS) \
	$(INC)/sys/types.h \
	$(INC)/sys/stat.h \
	$(INC)/sys/statvfs.h \
	$(INC)/errno.h $(INC)/sys/errno.h \
	$(INC)/unistd.h

io.o: io.c \
	$(LOCALHEADS) \
	dup.h \
	$(INC)/fcntl.h

jobs.o: jobs.c \
	$(INC)/sys/termio.h \
	$(INC)/sys/types.h \
	$(INC)/sys/wait.h \
	$(INC)/sys/param.h \
	$(INC)/string.h \
	$(INC)/fcntl.h \
	$(INC)/errno.h $(INC)/sys/errno.h \
	$(INC)/priv.h \
	$(INC)/pfmt.h \
	$(LOCALHEADS)

macro.o: macro.c \
	$(LOCALHEADS) \
	sym.h \
	$(INC)/wait.h

main.o: main.c \
	$(LOCALHEADS) \
	sym.h \
	timeout.h \
	$(INC)/sys/types.h \
	$(INC)/sys/stat.h \
	$(INC)/sys/wait.h \
	$(INC)/sys/resource.h \
	$(INC)/pfmt.h \
	hash.h \
	dup.h \
	$(INC)/sgtty.h

msg.o: msg.c \
	$(LOCALHEADS) \
	sym.h

name.o: name.c \
	$(INC)/pfmt.h \
	hash.h \
	$(LOCALHEADS) \
	$(INC)/sys/secsys.h

print.o: print.c \
	$(LOCALHEADS) \
	$(INC)/sys/param.h \
	$(INC)/pfmt.h

profile.o: profile.c

prv.o: prv.c \
	$(INC)/pfmt.h \
	$(INC)/priv.h \
	$(LOCALHEADS) \
	$(INC)/sys/secsys.h

pwd.o: pwd.c \
	mac.h \
	$(INC)/errno.h $(INC)/sys/errno.h \
	$(INC)/sys/types.h \
	$(INC)/sys/stat.h \
	$(INC)/limits.h \
	$(LOCALHEADS)

service.o: service.c \
	$(LOCALHEADS) \
	$(INC)/errno.h $(INC)/sys/errno.h \
	$(INC)/fcntl.h \
	$(INC)/memory.h \
	$(INC)/sys/types.h \
	$(INC)/sys/acct.h \
	$(INC)/sys/times.h

setbrk.o: setbrk.c \
	$(LOCALHEADS)

stak.o: stak.c \
	$(LOCALHEADS)

string.o: string.c \
	$(LOCALHEADS)

test.o: test.c \
	$(LOCALHEADS) \
	hash.h \
	$(INC)/sys/types.h \
	$(INC)/sys/stat.h \
	$(INC)/unistd.h

ulimit.o: ulimit.c \
	$(INC)/sys/resource.h \
	$(INC)/stdlib.h \
	$(INC)/pfmt.h \
	$(LOCALHEADS)

umask.o: umask.c \
	$(INC)/pfmt.h \
	$(LOCALHEADS)

word.o: word.c \
	$(LOCALHEADS) \
	sym.h

xec.o: xec.c \
	$(LOCALHEADS) \
	$(INC)/errno.h $(INC)/sys/errno.h \
	sym.h \
	hash.h \
	$(INC)/sys/types.h \
	$(INC)/sys/times.h

clean:
	rm -f $(OBJECTS)

clobber: clean
	rm -f $(MAINS)

lintit:
	$(LINT) $(LINTFLAGS) $(SOURCES)

install: all
	$(INS) -o -m 0555 -u $(OWN) -g $(GRP) -f $(SBIN) sh
	-@rm -rf $(SBIN)/OLDsh
	$(INS) -o -m 0555 -u $(OWN) -g $(GRP) -f $(USRBIN) sh.dy
	-/bin/mv $(USRBIN)/sh.dy $(USRBIN)/sh
	-@rm -rf $(USRBIN)/OLDsh
	$(CH)rm -f $(USRBIN)/jsh
	$(CH)ln -f $(USRBIN)/sh $(USRBIN)/jsh
	$(CH)rm -f $(USRLIB)/rsh
	$(CH)ln -f $(USRBIN)/sh $(USRLIB)/rsh
	if [ ! -d $(ETC)/default ] ; \
	then \
		mkdir $(ETC)/default ; \
	fi
	cp sh.dfl $(ETC)/default/sh

#	These targets are useful but optional

partslist:
	@echo sh.mk $(SOURCES) $(LOCALINCS) | tr ' ' '\012' | sort

productdir:
	@echo $(SBIN) | tr ' ' '\012' | sort

product:
	@echo sh | tr ' ' '\012' | \
	sed 's;^;$(SBIN)/;'

srcaudit:
	@fileaudit sh.mk $(LOCALINCS) $(SOURCES) -o $(OBJECTS) sh
