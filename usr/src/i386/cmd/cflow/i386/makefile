#ident	"@(#)cflow:i386/makefile	1.5"

include $(CMDRULES)

CMDBASE=	../..
COMDIR=		../common
SGSBASE=	$(CMDBASE)/sgs
SGSINC=		$(SGSBASE)/inc
SOURCES=	$(COMDIR)/dag.c $(COMDIR)/lpfx.c $(COMDIR)/nmf.c $(COMDIR)/flip.c
OBJECTS=	dag.o lpfx.o nmf.o flip.o
PRODUCTS=	dag lpfx nmf flip
INS=$(CMDBASE)/install/install.sh
LINTDIR=	$(SGSBASE)/alint/common
INCLIST=	-I$(LINTDIR) -I$(INC)
CC_CMD=		$(CC) $(CFLAGS) $(INCLIST) 
SED=		sed

all:	dag lpfx nmf flip

install:	all
	cp dag dag.bak
	cp lpfx lpfx.bak
	cp nmf nmf.bak
	cp flip flip.bak
	rm -f cflow cflow.uw
	cp $(COMDIR)/cflow.sh cflow
	chmod 644 cflow
	$(STRIP) dag
	$(STRIP) lpfx
	$(STRIP) nmf
	$(STRIP) flip
	/bin/sh $(INS) -f $(CCSLIB) dag
	/bin/sh $(INS) -f $(CCSLIB) lpfx
	/bin/sh $(INS) -f $(CCSLIB) nmf
	/bin/sh $(INS) -f $(CCSLIB) flip
	/bin/sh $(INS) -f $(CCSBIN) cflow
	# edit for gemini SDK on UnixWare and OpenServer
	cp $(COMDIR)/cflow.sh cflow.uw
	chmod 644 cflow.uw
	$(SED) -e "s+/usr/ccs/lib+$(ALT_PREFIX)/usr/ccs/lib+" cflow.uw >cflow
	/bin/sh $(INS) -f $(UW_CCSLIB) dag
	/bin/sh $(INS) -f $(UW_CCSLIB) lpfx
	/bin/sh $(INS) -f $(UW_CCSLIB) nmf
	/bin/sh $(INS) -f $(UW_CCSLIB) flip
	/bin/sh $(INS) -f $(UW_CCSBIN) cflow
	/bin/sh $(INS) -f $(OSR5_CCSLIB) dag
	/bin/sh $(INS) -f $(OSR5_CCSLIB) lpfx
	/bin/sh $(INS) -f $(OSR5_CCSLIB) nmf
	/bin/sh $(INS) -f $(OSR5_CCSLIB) flip
	/bin/sh $(INS) -f $(OSR5_CCSBIN) cflow
	mv dag.bak dag
	mv lpfx.bak lpfx
	mv nmf.bak nmf
	mv flip.bak flip
	rm -f cflow
	rm -f cflow.uw

dag:	$(COMDIR)/dag.c
	$(CC) $(CFLAGS) -I$(INC) -o dag $(COMDIR)/dag.c $(LINK_MODE)

lpfx:	$(COMDIR)/lpfx.c $(LINTDIR)/lnstuff.h $(SGSINC)/$(CPU)/sgs.h
	$(CC_CMD) -I$(SGSINC)/$(CPU) -o lpfx $(COMDIR)/lpfx.c $(LINK_MODE)

nmf:	$(COMDIR)/nmf.c
	$(CC) $(CFLAGS) -I$(INC) -o nmf $(COMDIR)/nmf.c $(LINK_MODE)

flip:	$(COMDIR)/flip.c
	$(CC) $(CFLAGS) -I$(INC) -o flip $(COMDIR)/flip.c $(LINK_MODE)

clean:
	rm -f *.o a.out make.out core

clobber:	clean
	rm -f dag lpfx nmf flip

lintit:
	$(LINT) $(LINTFLAGS) -I$(INC) $(COMDIR)/dag.c
	$(LINT) $(LINTFLAGS) $(INCLIST) -I$(SGSINC)/$(CPU) $(COMDIR)/lpfx.c
	$(LINT) $(LINTFLAGS) -I$(INC) $(COMDIR)/nmf.c
	$(LINT) $(LINTFLAGS) -I$(INC) $(COMDIR)/flip.c
