#ident	"@(#)lprof:cmd/cmd.mk	1.9.1.17"
#
# makefile for lprof command
#

include $(CMDRULES)

SGSBASE	= ../..
INS	= $(SGSBASE)/sgs.install

LD2DIR	= $(SGSBASE)/libdwarf2/$(CPU)
LIBD2	= $(LD2DIR)/libdwarf2.a

LELFDIR	= $(SGSBASE)/libelf/$(CPU)
LIBELF	= $(LELFDIR)/libelf.a

LPRDIR	= $(SGSBASE)/lprof/libprof/$(CPU)
LIBPROF	= $(LPRDIR)/libprof.a

DBG	= #-gvDDEBUG
OBJS	= dwarf1.o dwarf2.o gather.o main.o merge.o reduce.o report.o util.o
INCS	= -I$(CPUINC) -I$(COMINC) -I$(LPRDIR)
LIBS	= $(LIBD2) $(LIBELF) $(LIBPROF)

LPROF	= ../lprof

.c.o:
	$(CC) -c $(CFLAGS) $(DBG) $(INCS) $<

all: $(LPROF)

$(LPROF): $(OBJS) $(LIBPROF)
	$(CC) $(CFLAGS) $(DBG) $(INCS) -o $@ $(OBJS) $(LIBS)

$(OBJS): lprof.h

$(LIBPROF):
	cd $(LPRDIR); $(MAKE) -f libprof.mk

install: all
	cp $(LPROF) lprof.bak
	$(STRIP) $(LPROF)
	/bin/sh $(INS) 755 $(OWN) $(GRP) $(CCSBIN)/$(SGS)lprof $(LPROF)
	mv lprof.bak $(LPROF)

clean:
	rm -f $(OBJS)

clobber: clean
	rm -f $(LPROF)
