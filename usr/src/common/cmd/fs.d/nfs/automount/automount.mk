#ident	"@(#)automount.mk	1.3"
#ident	"$Header$"

#
# Make file for automount
#

include $(CMDRULES)

BINS= automount

OBJS= nfs_prot.o nfs_server.o nfs_trace.o nfs_cast.o \
	auto_main.o auto_look.o auto_proc.o auto_node.o \
	auto_mount.o auto_all.o mountxdr.o bindresvport.o ns.o
#	innetgr.o

SRCS= $(OBJS:.o=.c)
HDRS= automount.h nfs_prot.h
COFFLIBS= -lsocket -lnsl_s -lgen -lthread
ELFLIBS= -lsocket -lnsl -lgen -lthread
LDLIBS=`if [ x$(CCSTYPE) = xCOFF ] ; then echo "$(COFFLIBS)" ; else echo "$(ELFLIBS)" ; fi`

INCSYS= $(INC)
LOCALDEF= -D_REENTRANT

LINTFLAGS= -hbax $(DEFLIST) $(INCLIST)

# For debugging
#DEVINC1= -I.
#LDFLAGS= -g
#CFLAGS= -g
#CFLAGS= -P

INSDIR = $(USRLIB)/nfs
OWN = bin
GRP = bin

all: $(BINS)

$(BINS): $(OBJS)
	$(CC) -o $@ $(OBJS) $(LDFLAGS) $(LDLIBS) $(SHLIBS)

#nfs_prot.c: nfs_prot.h nfs_prot.x
#	rpcgen -c nfs_prot.x -o $@

#nfs_prot.h: nfs_prot.x
#	rpcgen -h nfs_prot.x -o $@

install: all
	[ -d $(INSDIR) ] || mkdir -p $(INSDIR)
	$(INS) -f $(INSDIR) -m 0555 -u $(OWN) -g $(GRP) $(BINS)

tags: $(SRCS)
	ctags $(SRCS)

lintit: $(SRCS)
	$(LINT) $(LOCALDEF) $(LINTFLAGS) $(SRCS)

clean:
	-rm -f $(OBJS)

clobber: clean
	-rm -f $(BINS)
