#ident	"@(#)cb:i386/makefile	1.5"
#
#	CB MAKEFILE
#
#

include $(CMDRULES)

LINT_CMD=$(LINT) $(USRINC)

INSDIR=$(CCSBIN)
LDLIBS=
CMDBASE=../..
INS=$(CMDBASE)/install/install.sh
SGSBASE=../../sgs
COMDIR=../common
CPUINC=$(SGSBASE)/inc/$(CPU)
HFILE= $(CPUINC)/sgs.h
INCLIST= -I$(CPUINC) -I$(INC)

CFILE = $(COMDIR)/cb.c
PRODUCTS=cb

all:	$(PRODUCTS)

$(PRODUCTS):	cb.o
	$(CC) cb.o $(LINK_MODE) -o $(PRODUCTS) $(LDLIBS)

cb.o:	$(CFILE) $(HFILE)
	$(CC) $(CFLAGS) $(INCLIST) -c $(CFILE) 


install: all
	 cp cb cb.bak
	 $(STRIP) cb
	 /bin/sh $(INS) -f $(INSDIR) cb
	 /bin/sh $(INS) -f $(UW_CCSBIN) cb
	 /bin/sh $(INS) -f $(OSR5_CCSBIN) cb
	 mv cb.bak cb

clean:
	 -rm -f *.o

clobber: clean
	-rm -f cb

lintit:	$(HFILE) $(CFILE)
	$(LINT_CMD) $(INCLIST) $(CFILE)
