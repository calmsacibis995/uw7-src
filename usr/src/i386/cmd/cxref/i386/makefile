#ident	"@(#)cxref:i386/makefile	1.7"

include $(CMDRULES)

LIBELF=
LDLIBS=
LINK_MODE=
LINTFLAGS=
SED=sed
CPUINC=../../sgs/inc/$(CPU)
INS=../../install/install.sh
INSDIR=$(CCSBIN)
COMDIR=../common
SOURCES=$(COMDIR)/cxref.c $(COMDIR)/st.c
OBJECTS=cxref.o st.o
PRODUCTS=xref

all:		$(PRODUCTS)

xref:		$(OBJECTS)
		$(CC) $(OBJECTS) $(LINK_MODE) $(LDLIBS) -o xref

cxref.o:	$(COMDIR)/cxref.c $(COMDIR)/cxref.h
		$(CC) $(CFLAGS) -I$(CPUINC) -I$(INC) -c $(COMDIR)/cxref.c

st.o:		$(COMDIR)/st.c
		$(CC) $(CFLAGS) -I$(INC) -c $(COMDIR)/st.c

install:	all
		cp xref xref.bak
		$(STRIP) xref
		rm -f cxref compat_cxref
		cp $(COMDIR)/cxref.sh cxref
		chmod 644 cxref
		/bin/sh $(INS) -f $(INSDIR) cxref
		/bin/sh $(INS) -f $(CCSLIB) xref
		# edit for gemini SDK on UnixWare and OpenServer
		cp cxref compat_cxref
		chmod 644 compat_cxref
		$(SED) -e "s+/usr/ccs/lib+$(ALT_PREFIX)/usr/ccs/lib+" compat_cxref >cxref
		/bin/sh $(INS) -f $(UW_CCSBIN) cxref
		/bin/sh $(INS) -f $(UW_CCSLIB) xref
		/bin/sh $(INS) -f $(OSR5_CCSBIN) cxref
		/bin/sh $(INS) -f $(OSR5_CCSLIB) xref
		rm -f cxref 
		rm -f compat_cxref 
		mv xref.bak xref

lintit:		$(SOURCES)
		$(LINT) $(LINTFLAGS) -I$(CPUINC) -I$(INC) $(SOURCES)

clean:
		-rm -f $(OBJECTS)

clobber:	clean
		-rm -f $(PRODUCTS)
