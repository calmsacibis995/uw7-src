#ident	"@(#)bootpd.mk	1.2"
#ident	"$Header$"

include $(CMDRULES)

INCSYS = $(INC)
BINS= bootparamd
OBJS= bp_svc.o bp_subr.o bp_lib.o bp_xdr.o
SRCS= $(OBJS:.o=.c)
INSDIR = $(USRLIB)/nfs
OWN = bin
GRP = bin

LINTFLAGS= -hbax $(DEFLIST)
COFFLIBS= -lnsl_s -lsocket
ELFLIBS = -lnsl -lsocket
LDLIBS=`if [ x$(CCSTYPE) = xCOFF ] ; then echo "$(COFFLIBS)" ; else echo "$(ELFLIBS)" ; fi`

all: $(BINS)

$(BINS): $(OBJS)
	$(CC) -o $@ $(OBJS) $(LDFLAGS) $(LDLIBS) $(SHLIBS)

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
