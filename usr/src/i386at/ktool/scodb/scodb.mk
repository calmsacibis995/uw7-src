#ident	"@(#)ktool:i386at/ktool/scodb/scodb.mk	1.3"
include $(CMDRULES)


#CFLAGS=-O -I../../uts
CFLAGS=-O  $(INCLIST)

INSDIR=$(ROOT)/$(MACH)/etc/conf/pack.d/scodb/info

all: make_info make_idef lineno info_to_c

make_idef:	make_idef.o stun.o pfe.o
	$(CC) -o make_idef make_idef.o stun.o pfe.o -lelf

lineno: lineno.o
	$(CC) -o lineno lineno.o -lelf

info_to_c: info_to_c.o
	$(CC) -o info_to_c info_to_c.o

make_info:	make_info.o dump.o pfe.o stun.o local.o
	$(CC) -o make_info make_info.o dump.o pfe.o stun.o local.o -lelf

install: all
	-[ -d $(INSDIR) ] || mkdir -p $(INSDIR)
	$(INS) -f $(INSDIR) -m $(BINMODE) -u $(OWN) -g $(GRP) make_idef
	$(INS) -f $(INSDIR) -m $(BINMODE) -u $(OWN) -g $(GRP) make_info
	$(INS) -f $(INSDIR) -m $(BINMODE) -u $(OWN) -g $(GRP) lineno
	$(INS) -f $(INSDIR) -m $(BINMODE) -u $(OWN) -g $(GRP) idbuild_hook
	$(INS) -f $(INSDIR) -m $(BINMODE) -u $(OWN) -g $(GRP) info_to_c
	$(INS) -f $(INSDIR) -m 444 -u $(OWN) -g $(GRP) README

clobber: clean

clean:
	rm -f *.o make_idef lineno make_info info_to_c
