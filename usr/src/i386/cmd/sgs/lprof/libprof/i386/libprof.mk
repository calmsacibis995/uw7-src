#ident	"@(#)lprof:libprof/i386/libprof.mk	1.7"
#
#	makefile for libprof.a
#
# libprof.a is the runtime support for both regular and line profiling
#

include $(CMDRULES)

SGSBASE	= ../../..
INS	= $(SGSBASE)/sgs.install

PLBBASE	= ../../libprof
COMDIR	= $(PLBBASE)/common
CPUDIR	= $(PLBBASE)/$(CPU)
INCBASE	= ../../hdr

OBJS	= SOinout.o cntmerge.o dprofil.o mcount.o newdump.o newmon.o

SRCS	= $(COMDIR)/SOinout.c $(COMDIR)/cntmerge.c $(COMDIR)/dprofil.c \
	$(CPUDIR)/mcount.s $(COMDIR)/newdump.c $(COMDIR)/newmon.c

HDRS	= $(CPUDIR)/mach_type.h

INCS	= -I$(CPUDIR) -I$(COMDIR) -I$(INCBASE) -I$(CPUINC) -I$(COMINC)

LIBPROF	= libprof.a
PRODS	= $(LIBPROF)

all: $(PRODS)
	if test ! "$(PROF_SAVE)"; then rm -f $(OBJS); fi

$(LIBPROF): $(OBJS)
	$(AR) $(ARFLAGS) $(LIBPROF) `$(LORDER) $(OBJS) | tsort`

SOinout.o: $(LPRDHRS) $(COMDIR)/SOinout.c
	$(CC) -c $(CFLAGS) $(INCS) $(COMDIR)/SOinout.c
cntmerge.o: $(LPRDHRS) $(COMDIR)/cntmerge.c
	$(CC) -c $(CFLAGS) $(INCS) $(COMDIR)/cntmerge.c
dprofil.o: $(LPRDHRS) $(COMDIR)/dprofil.c
	$(CC) -c $(CFLAGS) $(INCS) $(COMDIR)/dprofil.c
mcount.o: $(LPRDHRS) $(CPUDIR)/mcount.s
	$(CC) -c $(CFLAGS) $(INCS) $(CPUDIR)/mcount.s
newdump.o: $(LPRDHRS) $(COMDIR)/newdump.c
	$(CC) -c $(CFLAGS) $(INCS) $(COMDIR)/newdump.c
newmon.o: $(LPRDHRS) $(COMDIR)/newmon.c
	$(CC) -c $(CFLAGS) $(INCS) $(COMDIR)/newmon.c

install: all
	/bin/sh $(INS) 644 $(OWN) $(GRP) $(CCSLIB)/libprof.a $(LIBPROF)

lintit:	$(SRCS)
	$(LINT) $(LINTFLAGS) $(SRCS)

clean:
	rm -f $(OBJS) *.ln

clobber: clean
	rm -f $(PRODS)
