#	copyright	"%c%"

#ident	"@(#)ufs.cmds:common/cmd/fs.d/ufs/ufsrestore/ufsrestore.mk	1.10.6.4"
#ident "$Header$"

include $(CMDRULES)

# LOCALDEF:
#       DEBUG                   use local directory to find ddate and dumpdates
#       TDEBUG                  trace out the process forking
#
MAINS = ufsrestore ufsrestore.stat

OBJS= dirs.o interactive.o main.o restore.o symtab.o \
	tape.o utilities.o
SRCS= dirs.c interactive.c main.c restore.c symtab.c \
	tape.c utilities.c
HDRS= dump.h

INSDIR = $(USRLIB)/fs/ufs
OWN = bin
GRP = bin
LDLIBS = -lgen

RM= rm -f


all: $(MAINS)

ufsrestore: $(OBJS)
	$(CC) $(LDFLAGS) -o $@ $(OBJS) $(LDLIBS) $(SHLIBS)

ufsrestore.stat: $(OBJS)
	$(CC) $(LDFLAGS) -o $@ $(OBJS) $(LDLIBS) $(NOSHLIBS)

install: all
	[ -d $(INSDIR) ] || mkdir -p $(INSDIR)
	$(INS) -f $(INSDIR) -m 0555 -u $(OWN) -g $(GRP) ufsrestore
	$(INS) -f $(INSDIR) -m 0555 -u $(OWN) -g $(GRP) ufsrestore.stat
	$(INS) -f $(USRSBIN) -m 0555 -u $(OWN) -g $(GRP) ufsrestore
	
clean:
	$(RM) $(BINS) $(OBJS)

clobber: clean
	$(RM) ufsrestore
