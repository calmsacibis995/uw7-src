#ident	"@(#)fpe387:fpemul.mk	1.1"
#ident	"$Header$"

include $(CMDRULES)

OWN = bin
GRP = bin

INSDIR = $(ROOT)/$(MACH)/etc
INSMODE = 644

OBJS =	dcode.o \
	arith.o \
	divmul.o \
	lipsq.o \
	reg.o \
	remsc.o \
	round.o \
	status.o \
	store.o \
	subadd.o \
	trans.o

all:	emulator

emulator: $(OBJS) 
	$(LD) $(LDFLAGS) -Mmapfile -e e80387 -o $@ $(OBJS) $(ROOTLIBS)

install: all
	[ -d $(INSDIR) ] || mkdir -p $(INSDIR)
	$(INS) -f $(INSDIR) -m $(INSMODE) -u $(OWN) -g $(GRP) emulator

clean:
	-rm -f $(OBJS)

clobber: clean
	-rm emulator

$(OBJS):	e80387.m4

FRC: 
