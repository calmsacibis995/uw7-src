#ident	"@(#)unix_conv:i386/makefile	1.5"

#***************************************************************************
#                                                                          *
#		UNIX 5.0 Transition Aid (convert) makefile	 	   *
#									   *
#***************************************************************************
#                                                                          *
#		Header/Data Files					   *
#                                                                          *
#	inc/ar.h		archive file format header		   *
#	inc/a.out.h		machine language file format header        *
#	inc/sgs.h		sgs primitives				   *
#	com/old.a.out.h		pre 5.0 headers for a.out.h		   *
#	com/old.ar.h		pre 5.0 headers for ar.h		   *
#                                                                          *
#***************************************************************************
#                                                                          *
#		Source Code Files					   *
#								           *
#	com/convert.c		transition aid source code		   *
#									   *
#***************************************************************************

include $(CMDRULES)

LPT = dpr -mx

OUTFIL = output.tmp


OFILES = convert.o

CFILES = $(COMDIR)/convert.c

HFILES = $(COMINC)/ar.h $(COMINC)/aouthdr.h $(COMINC)/filehdr.h \
	 $(COMINC)/scnhdr.h $(COMINC)/linenum.h $(COMINC)/reloc.h \
	 $(COMINC)/syms.h $(COMINC)/storclass.h \
	 $(COMDIR)/old.a.out.h $(COMDIR)/old.ar.h 5.0.ar.h \
	 $(CPUINC)/paths.h

DEFLIST	= -D$(CPU) $(FLEX) -D$(ARFORMAT)
INCLIST	= -I. -I$(COMDIR) -I$(COMINC) -I$(CPUINC)

all:	convert


convert:$(CFILES) $(HFILES) $(LIBLD)
	$(CC) $(DEFLIST) $(INCLIST) $(CFLAGS) $(LINK_MODE) $(CFILES) $(LIBLD) $(LDLIBS) -o convert


install:	convert
	cp convert convert.bak
	$(STRIP) convert
	sh $(SGSBASE)/sgs.install 755 $(OWN) $(GRP) $(CCSBIN)/convert convert
	mv convert.bak convert

listing:
	pr -n makefile $(HFILES) $(CFILES) | $(LPT)
	@echo "files sent to lineprinter"

clean:
	-rm -f $(OFILES)

flow:
	cflow $(PPFLAGS) $(CFILES) > $(OUTFIL)

flowr:
	cflow -r $(PPFLAGS) $(CFILES) > $(OUTFIL)

shrink:  clobber

clobber: clean
	-rm -f convert
