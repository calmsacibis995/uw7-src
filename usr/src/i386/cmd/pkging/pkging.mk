#	copyright	"%c%"

#ident	"@(#)pkging.mk	15.1"
#ident  "$Header$"

include $(CMDRULES)


OWN = 
GRP = 

ARCH = AT386
BUS = AT386
# ROOTLIBS = -dn
# CFLAGS = -O -D$(ARCH) -D$(BUS) 
# INS=install
FILES = installpkg message removepkg displaypkg Install.sh removepkg.r \
	installpkg.r flop_disk Install.tape get_sel flop_num

all: $(FILES)

flop_num: $$@.o
	$(CC) -o $@ $@.o $(LDFLAGS) $(LDLIBS) $(ROOTLIBS)

message flop_disk get_sel: $$@.o
	$(CC) -o $@ $@.o $(LDFLAGS) $(LDLIBS) $(SHLIBS)

flop_disk.o: flop_disk.c \
	$(INC)/stdio.h \
	$(INC)/termio.h $(INC)/sys/termio.h \
	$(INC)/fcntl.h $(INC)/sys/fcntl.h \
	$(INC)/signal.h $(INC)/sys/signal.h

flop_num.o: flop_num.c \
	$(INC)/fcntl.h $(INC)/sys/fcntl.h

get_sel.o: get_sel.c \
	$(INC)/stdio.h \
	$(INC)/termio.h $(INC)/sys/termio.h \
	$(INC)/fcntl.h $(INC)/sys/fcntl.h

message.o: message.c \
	$(INC)/stdio.h \
	$(INC)/termio.h $(INC)/sys/termio.h \
	$(INC)/signal.h $(INC)/sys/signal.h

install: all
	$(INS) -f $(USRBIN) installpkg
	$(INS) -f $(USRBIN) message
	$(INS) -f $(USRBIN) removepkg
	$(INS) -f $(USRBIN) displaypkg
	$(INS) -f $(USRLBIN) Install.sh
	$(INS) -f $(USRLBIN) installpkg.r
	$(INS) -f $(USRLBIN) removepkg.r
	$(INS) -f $(USRLBIN) Install.tape
	$(INS) -f $(USRLBIN) get_sel
	$(INS) -f $(USRLBIN) ADD.base.pkg
	$(INS) -f $(USRLBIN) RM.base.pkg
	$(INS) -f $(USRSBIN) flop_disk
	$(INS) -f $(SBIN) flop_num
	if [ -d $(ROOT)/$(MACH)/xenv ] ; \
	then \
		$(HCC) -s -o $(ROOT)/$(MACH)/xenv/flop_num flop_num.c;\
	fi

clean clobber:
	rm -f message.o message flop_disk.o flop_disk get_sel.o get_sel \
		flop_num flop_num.o

lintit:
	$(LINT) $(LINTFLAGS) flop_disk.c
	$(LINT) $(LINTFLAGS) flop_num.c
	$(LINT) $(LINTFLAGS) get_sel.c
	$(LINT) $(LINTFLAGS) message.c
