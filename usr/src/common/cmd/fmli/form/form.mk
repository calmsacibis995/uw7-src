#ident	"@(#)fmli:form/form.mk	1.7.5.2"

include $(CMDRULES)

LIBRARY=libform.a
HEADER1=../inc
LOCALINC=-I$(HEADER1)
OBJECTS= fcheck.o \
	fclose.o \
	fctl.o \
	fcurrent.o \
	fcustom.o \
	fdefault.o \
	frefresh.o

$(LIBRARY): $(OBJECTS)
	$(AR) $(ARFLAGS) $(LIBRARY) $(OBJECTS)

fcheck.o: $(HEADER1)/form.h
fcheck.o: $(HEADER1)/token.h
fcheck.o: $(HEADER1)/winp.h
fcheck.o: $(HEADER1)/wish.h
fcheck.o: fcheck.c

fclose.o: $(HEADER1)/form.h
fclose.o: $(HEADER1)/token.h
fclose.o: $(HEADER1)/var_arrays.h
fclose.o: $(HEADER1)/vtdefs.h
fclose.o: $(HEADER1)/winp.h
fclose.o: $(HEADER1)/wish.h
fclose.o: fclose.c

fctl.o: $(HEADER1)/ctl.h
fctl.o: $(HEADER1)/form.h
fctl.o: $(HEADER1)/token.h
fctl.o: $(HEADER1)/vtdefs.h
fctl.o: $(HEADER1)/winp.h
fctl.o: $(HEADER1)/wish.h
fctl.o: fctl.c

fcurrent.o: $(HEADER1)/form.h
fcurrent.o: $(HEADER1)/token.h
fcurrent.o: $(HEADER1)/vtdefs.h
fcurrent.o: $(HEADER1)/winp.h
fcurrent.o: $(HEADER1)/wish.h
fcurrent.o: fcurrent.c

fcustom.o: $(HEADER1)/form.h
fcustom.o: $(HEADER1)/token.h
fcustom.o: $(HEADER1)/var_arrays.h
fcustom.o: $(HEADER1)/winp.h
fcustom.o: $(HEADER1)/wish.h
fcustom.o: fcustom.c

fdefault.o: $(HEADER1)/ctl.h
fdefault.o: $(HEADER1)/form.h
fdefault.o: $(HEADER1)/terror.h
fdefault.o: $(HEADER1)/token.h
fdefault.o: $(HEADER1)/vtdefs.h
fdefault.o: $(HEADER1)/winp.h
fdefault.o: $(HEADER1)/wish.h
fdefault.o: fdefault.c

frefresh.o: $(HEADER1)/attrs.h
frefresh.o: $(HEADER1)/form.h
frefresh.o: $(HEADER1)/token.h
frefresh.o: $(HEADER1)/winp.h
frefresh.o: $(HEADER1)/wish.h
frefresh.o: frefresh.c

##### Standard makefile targets ######

all:		$(LIBRARY)

install:	all

clean:
		/bin/rm -f *.o

clobber:	clean
		/bin/rm -f $(LIBRARY)

.PRECIOUS:	$(LIBRARY)
