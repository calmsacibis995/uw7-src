#ident	"@(#)cvtomf:cvtomf.mk	1.2"

#	This Module contains Proprietary Information of Microsoft
#	Corporation and should be treated as Confidential.

#    Portions Copyright 1983-1989 The Santa Cruz Operation, Inc
#		      All Rights Reserved

# Enhanced Application Compatibility Support

# Changed ARCH to ARCH_TYPE to remove conflict with build variable of the
# same name

include	$(CMDRULES)

ARCH_TYPE = AR32WR
NPROC	= ONEPROC
FLEX	= -DFLEXNAMES
SCOINC	= ../sco

LOCALINC = -I$(SCOINC) -I.
LOCALDEF = -D$(NPROC) -DARCH=F_$(ARCH_TYPE) -D$(ARCH_TYPE) $(FLEX) $(NATIVE)
LDLIBS	= -lmalloc
LIBS	=
LIBES	=


OBJECTS	= coff.o cvtomf.o omf.o proc_sym.o proc_typ.o fltused.o

CFILES	= coff.c cvtomf.c omf.c proc_sym.c proc_typ.c fltused.c

HFILES	= coff.h cvtomf.h omf.h symbol.h leaf.h $(SCOINC)/filehdr.h \
	  scnhdr.h linenum.h aouthdr.h reloc.h $(SCOINC)/syms.h \
	  storclass.h sgs.h 

SHFILES = cvtomflib.sh


all: cvtomf cvtomflib fltused.o

cvtomf: $(OBJECTS)
	$(CC) -o cvtomf $(OBJECTS) $(LDFLAGS) $(LDLIBS)

coff.o: coff.c coff.h cvtomf.h 

cvtomf.o: cvtomf.c cvtomf.h 

omf.o: omf.c omf.h cvtomf.h 

proc_sym.o: proc_sym.c symbol.h cvtomf.h leaf.h

proc_typ.o: proc_typ.c symbol.h cvtomf.h leaf.h

cvtomflib: cvtomflib.sh
	cp cvtomflib.sh cvtomflib
	chmod 755 cvtomflib

install: all
	$(INS) -f $(USRBIN) -m 511 -u bin -g bin cvtomf
	$(INS) -f $(USRBIN) -m 555 -u bin -g bin cvtomflib
	$(INS) -f $(USRLIB) -m 644 -u bin -g bin fltused.o

clean:
	rm -f $(OBJECTS)

clobber: clean
	rm -f cvtomf cvtomflib
# End Enhanced Application Compatibility Support
