#ident	"@(#)fmli:oeu/oeu.mk	1.15.4.2"

include $(CMDRULES)

LIBRARY=liboeu.a
HEADER1=../inc
LOCALINC=-I$(HEADER1)
LOCALDEF=-DJUSTCHECK
OBJECTS= oeu.o \
	oeucheck.o \
	genparse.o

$(LIBRARY): oeucheck.c $(OBJECTS)
	$(AR) $(ARFLAGS) $(LIBRARY) $(OBJECTS)
	/bin/rm -f oeucheck.o
	/bin/rm -f oeucheck.c


genparse.o: $(HEADER1)/eft.types.h
genparse.o: $(HEADER1)/inc.types.h
genparse.o: $(HEADER1)/io.h
genparse.o: $(HEADER1)/mail.h
genparse.o: $(HEADER1)/moremacros.h
genparse.o: $(HEADER1)/parse.h
genparse.o: $(HEADER1)/retcodes.h
genparse.o: $(HEADER1)/sizes.h
genparse.o: $(HEADER1)/smdef.h
genparse.o: $(HEADER1)/terror.h
genparse.o: $(HEADER1)/typetab.h
genparse.o: $(HEADER1)/wish.h
genparse.o: genparse.c

oeu.o: $(HEADER1)/eft.types.h
oeu.o: $(HEADER1)/inc.types.h
oeu.o: $(HEADER1)/io.h
oeu.o: $(HEADER1)/mail.h
oeu.o: $(HEADER1)/parse.h
oeu.o: $(HEADER1)/partabdefs.h
oeu.o: $(HEADER1)/retcodes.h
oeu.o: $(HEADER1)/smdef.h
oeu.o: $(HEADER1)/sizes.h
oeu.o: $(HEADER1)/terror.h
oeu.o: $(HEADER1)/typetab.h
oeu.o: $(HEADER1)/wish.h
oeu.o: oeu.c

oeucheck.o: $(HEADER1)/eft.types.h
oeucheck.o: $(HEADER1)/inc.types.h
oeucheck.o: $(HEADER1)/io.h
oeucheck.o: $(HEADER1)/mail.h
oeucheck.o: $(HEADER1)/parse.h
oeucheck.o: $(HEADER1)/partabdefs.h
oeucheck.o: $(HEADER1)/retcodes.h
oeucheck.o: $(HEADER1)/smdef.h
oeucheck.o: $(HEADER1)/terror.h
oeucheck.o: $(HEADER1)/typetab.h
oeucheck.o: $(HEADER1)/wish.h
oeucheck.o: oeu.c

oeucheck.c: 
	/bin/cp oeu.c oeucheck.c

###### Standard makefile targets ######
all:		$(LIBRARY)

install:	all

clean:
		/bin/rm -f *.o

clobber:	clean
		/bin/rm -f $(LIBRARY)

.PRECIOUS:	$(LIBRARY)
