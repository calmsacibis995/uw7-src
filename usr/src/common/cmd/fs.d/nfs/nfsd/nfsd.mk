#ident	"@(#)nfsd.mk	1.2"
#ident	"$Header$"

include $(CMDRULES)

BINS= nfsd
OBJS= nfsd.o
SRCS= $(OBJS:.o=.c)
INCSYS = $(INC)
INSDIR = $(USRLIB)/nfs
OWN = bin
GRP = bin

LOCALDEF= -Dnetselstrings 
LINTFLAGS= -hbax $(DEFLIST)
COFFLIBS= -lnsl_s -lsocket
ELFLIBS = -lnsl
LDLIBS=`if [ x$(CCSTYPE) = xCOFF ] ; then echo "$(COFFLIBS)" ; else echo "$(ELFLIBS)" ; fi`

all: $(BINS)

$(BINS): $(OBJS)
	$(CC) -o $@ $(OBJS) $(LDFLAGS) $(SHLIBS) $(LDLIBS)

install: all
	[ -d $(INSDIR) ] || mkdir -p $(INSDIR)
	$(INS) -f $(INSDIR) -m 0555 -u $(OWN) -g $(GRP) $(BINS)

lintit:
	$(LINT) $(LINTFLAGS) $(SRCS)

tags: $(SRCS)
	ctags $(SRCS)

clean:
	-rm -f $(OBJS)

clobber: clean
	-rm -f $(BINS)
