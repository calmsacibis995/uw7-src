#ident	"@(#)lockd.mk	1.2"
#ident	"$Header$"

include $(CMDRULES)

OWN = bin
GRP = bin

BINS= lockd
OBJS= xdr_klm.o xdr_nlm.o xdr_sm.o prot_pklm.o prot_msg.o prot_lock.o \
      prot_alloc.o prot_free.o prot_share.o hash.o \
      prot_proc.o prot_libr.o prot_pnlm.o \
      prot_priv.o  sm_monitor.o prot_main.o \
      rpc.o svc_create.o

# pmap_clnt.o pmap_prot.o rpc_soc.o svc.o ti_opts.o svc_dg.o

SRCS= $(OBJS:.o=.c)
INCSYS = $(INC)
INSDIR = $(USRLIB)/nfs

LINTFLAGS= -hbax $(DEFLIST)
COFFLIBS= -lrpc -ldes -lnsl_s -lsocket -lgen
#ELFLIBS = -lrpcsvc -dy -lnsl -lrpc -ldes -lnet -lsocket -lgen
ELFLIBS = -L /usr/ucblib -dy -lnsl -lsocket -lgen
LDLIBS = `if [ x$(CCSTYPE) = xCOFF ] ; then echo "$(COFFLIBS)" ; else echo "$(ELFLIBS)" ; fi`

all: $(BINS)

$(BINS): $(OBJS)
	$(CC) -o $@ $(OBJS) $(LDFLAGS) $(SHLIBS) $(LDLIBS)

install: all
	[ -d $(INSDIR) ] || mkdir -p $(INSDIR)
	$(INS) -f $(INSDIR) -m 0555 -u $(OWN) -g $(GRP) $(BINS)

lintit:
	$(LINT) $(LINTFLAGS) $(SRCS)

tags: $(SRCS)
	ctags -tw $(SRCS)

clean:
	-rm -f $(OBJS)

clobber: clean
	-rm -f $(BINS)
