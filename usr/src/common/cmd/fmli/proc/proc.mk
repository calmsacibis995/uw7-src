#ident	"@(#)fmli:proc/proc.mk	1.11.4.2"

include $(CMDRULES)

LIBRARY = libproc.a
HEADER1=../inc
LOCALINC=-I$(HEADER1)
OBJECTS= pclose.o \
	pcurrent.o \
	pctl.o \
	pdefault.o \
	pnoncur.o \
	open.o 

$(LIBRARY):	$(OBJECTS)
	$(AR) $(ARFLAGS) $(LIBRARY) $(OBJECTS)

open.o: $(HEADER1)/eft.types.h
open.o: $(HEADER1)/inc.types.h
open.o: $(HEADER1)/actrec.h
open.o: $(HEADER1)/moremacros.h
open.o: $(HEADER1)/slk.h
open.o: $(HEADER1)/terror.h
open.o: $(HEADER1)/token.h
open.o: $(HEADER1)/wish.h
open.o: ./proc.h
open.o: open.c

pclose.o: $(HEADER1)/eft.types.h
pclose.o: $(HEADER1)/inc.types.h
pclose.o: $(HEADER1)/actrec.h
pclose.o: $(HEADER1)/procdefs.h
pclose.o: $(HEADER1)/slk.h
pclose.o: $(HEADER1)/terror.h
pclose.o: $(HEADER1)/token.h
pclose.o: $(HEADER1)/wish.h
pclose.o: ./proc.h
pclose.o: pclose.c

pctl.o: $(HEADER1)/eft.types.h
pctl.o: $(HEADER1)/inc.types.h
pctl.o: $(HEADER1)/actrec.h
pctl.o: $(HEADER1)/ctl.h
pctl.o: $(HEADER1)/procdefs.h
pctl.o: $(HEADER1)/sizes.h
pctl.o: $(HEADER1)/slk.h
pctl.o: $(HEADER1)/terror.h
pctl.o: $(HEADER1)/token.h
pctl.o: $(HEADER1)/wish.h
pctl.o: ./proc.h
pctl.o: pctl.c

pcurrent.o: $(HEADER1)/eft.types.h
pcurrent.o: $(HEADER1)/inc.types.h
pcurrent.o: $(HEADER1)/actrec.h
pcurrent.o: $(HEADER1)/procdefs.h
pcurrent.o: $(HEADER1)/slk.h
pcurrent.o: $(HEADER1)/sizes.h
pcurrent.o: $(HEADER1)/terror.h
pcurrent.o: $(HEADER1)/token.h
pcurrent.o: $(HEADER1)/wish.h
pcurrent.o: ./proc.h
pcurrent.o: pcurrent.c

pdefault.o: $(HEADER1)/eft.types.h
pdefault.o: $(HEADER1)/inc.types.h
pdefault.o: $(HEADER1)/actrec.h
pdefault.o: $(HEADER1)/procdefs.h
pdefault.o: $(HEADER1)/slk.h
pdefault.o: $(HEADER1)/terror.h
pdefault.o: $(HEADER1)/token.h
pdefault.o: $(HEADER1)/wish.h
pdefault.o: ./proc.h
pdefault.o: pdefault.c

pnoncur.o: $(HEADER1)/eft.types.h
pnoncur.o: $(HEADER1)/inc.types.h
pnoncur.o: $(HEADER1)/procdefs.h
pnoncur.o: $(HEADER1)/terror.h
pnoncur.o: $(HEADER1)/wish.h
pnoncur.o: ./proc.h
pnoncur.o: pnoncur.c

###### Standard makefile targets #######

all:		$(LIBRARY)

install:	all

clean:
		/bin/rm -f *.o

clobber:	clean
		/bin/rm -f $(LIBRARY)

.PRECIOUS:	$(LIBRARY)
