#	copyright	"%c%"

#ident	"@(#)ufs.cmds:common/cmd/fs.d/ufs/ufsdump/ufsdump.mk	1.12.6.3"
#ident "$Header$"

include $(CMDRULES)

#       dump.h                  header file
#       dumpitime.c             reads /etc/dumpdates
#       dumpmain.c              driver
#       dumpoptr.c              operator interface
#       dumptape.c              handles the mag tape and opening/closing
#       dumptraverse.c          traverses the file system
#       unctime.c               undo ctime
#
# LOCALDEF:
#       DEBUG                   use local directory to find ddate and dumpdates
#       TDEBUG                  trace out the process forking
#
BINS= ufsdump
OBJS= dumpitime.o dumpmain.o dumpoptr.o dumptape.o \
	dumptraverse.o unctime.o
SRCS= dumpitime.c dumpmain.c dumpoptr.c dumptape.c \
	dumptraverse.c unctime.c
HDRS= dump.h

INSDIR1 = $(USRLIB)/fs/ufs
INSDIR2 = $(USRSBIN)
OWN = bin
GRP = bin
LDLIBS = -lgen

all: $(BINS)

$(BINS): $(OBJS)
	$(CC) $(LDFLAGS) -o $@ $(OBJS) $(LDLIBS) $(SHLIBS)

install: $(BINS)
	[ -d $(INSDIR1) ] || mkdir -p $(INSDIR1)
	$(INS) -f $(INSDIR1) -m 0555 -u $(OWN) -g $(GRP) ufsdump
	-rm -f $(INSDIR2)/ufsdump
	ln $(INSDIR1)/ufsdump $(INSDIR2)/ufsdump
	
clean:
	rm -f $(BINS) $(OBJS)

clobber: clean
	rm -f $(BINS)
