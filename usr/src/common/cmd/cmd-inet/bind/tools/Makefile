#ident "@(#)Makefile	1.4"
#ident "$Header$"

include $(CMDRULES)

SRCS		= host.c dig.c dnsquery.c addr.c
HOST_O		= host.o
NSLDIG_O	= nslookup/list.o nslookup/subr.o nslookup/debug.o nslookup/send.o
DIG_O		= dig.o $(NSLDIG_O)
DNSQUERY_O	= dnsquery.o
ADDR_O		= addr.o
RES		= ../res/libresolv.a
LDLIBS		= $(RES) -lsocket -lnsl
LOCALDEF =  	-DSVR4 -DUSE_POSIX
LOCALINC =	-I../include

SUBDIRS = nslookup 
BINARIES = host dig dnsquery addr

INSDIR=	$(USRSBIN)
OWN=	bin
GRP=	bin

.MUTEX: dig nslookup

all: $(SUBDIRS) $(BINARIES)

install: all
	$(INS) -f $(INSDIR) -m 0555 -u $(OWN) -g $(GRP) dig
	$(INS) -f $(INSDIR) -m 0555 -u $(OWN) -g $(GRP) host
	$(INS) -f $(INSDIR) -m 0555 -u $(OWN) -g $(GRP) dnsquery
	@cd nslookup;\
	/bin/echo "\n===== $(MAKE) install";\
	$(MAKE) install $(MAKEARGS)

host: $(HOST_O)
	$(CC) -o $@ $(HOST_O) $(LDFLAGS) $(LDLIBS) $(PERFLIBS)

$(NSLDIG_O):
	(cd nslookup; $(MAKE) $(@F))

dig: $(DIG_O)
	$(CC) -o $@ $(DIG_O) $(LDFLAGS) $(LDLIBS) $(PERFLIBS)

dnsquery: $(DNSQUERY_O)
	$(CC) -o $@ $(DNSQUERY_O) $(LDFLAGS) $(LDLIBS) $(PERFLIBS)

addr: $(ADDR_O)
	$(CC) -o $@ $(ADDR_O) $(LDFLAGS) $(LDLIBS) $(PERFLIBS)

$(SUBDIRS): FRC
	cd $@; $(MAKE)

clean:
	@for x in $(SUBDIRS); do \
		(cd $$x; pwd; $(MAKE) clean); \
	done
	rm -f *.o

clobber: clean
	@for x in $(SUBDIRS); do \
		(cd $$x; pwd; $(MAKE) clobber); \
	done
	rm -f $(BINARIES)

FRC:
