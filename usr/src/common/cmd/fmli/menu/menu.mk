#ident	"@(#)fmli:menu/menu.mk	1.13.4.2"

include $(CMDRULES)

LIBRARY=libmenu.a
HEADER1=../inc
LOCALINC=-I$(HEADER1)
OBJECTS= mclose.o \
	mctl.o \
	mcurrent.o \
	mcustom.o \
	mfolder.o \
	mdefault.o \
	mreshape.o \
	stmenu.o

$(LIBRARY): $(OBJECTS)
	$(AR) $(ARFLAGS) $(LIBRARY) $(OBJECTS)

mclose.o: $(HEADER1)/var_arrays.h
mclose.o: $(HEADER1)/wish.h
mclose.o: $(HEADER1)/menu.h
mclose.o: mclose.c

mctl.o: $(HEADER1)/ctl.h
mctl.o: $(HEADER1)/menudefs.h
mctl.o: $(HEADER1)/wish.h
mctl.o: $(HEADER1)/menu.h
mctl.o: mctl.c

mcurrent.o: $(HEADER1)/attrs.h
mcurrent.o: $(HEADER1)/color_pair.h
mcurrent.o: $(HEADER1)/ctl.h
mcurrent.o: $(HEADER1)/menudefs.h
mcurrent.o: $(HEADER1)/sizes.h
mcurrent.o: $(HEADER1)/vtdefs.h
mcurrent.o: $(HEADER1)/wish.h
mcurrent.o: $(HEADER1)/menu.h
mcurrent.o: mcurrent.c

mcustom.o: $(HEADER1)/ctl.h
mcustom.o: $(HEADER1)/menudefs.h
mcustom.o: $(HEADER1)/var_arrays.h
mcustom.o: $(HEADER1)/vtdefs.h
mcustom.o: $(HEADER1)/wish.h
mcustom.o: $(HEADER1)/menu.h
mcustom.o: mcustom.c

mfolder.o: $(HEADER1)/ctl.h
mfolder.o: $(HEADER1)/menudefs.h
mfolder.o: $(HEADER1)/terror.h
mfolder.o: $(HEADER1)/vtdefs.h
mfolder.o: $(HEADER1)/wish.h
mfolder.o: $(HEADER1)/sizes.h
mfolder.o: $(HEADER1)/menu.h
mfolder.o: mfolder.c

mdefault.o: $(HEADER1)/ctl.h
mdefault.o: $(HEADER1)/menudefs.h
mdefault.o: $(HEADER1)/terror.h
mdefault.o: $(HEADER1)/vtdefs.h
mdefault.o: $(HEADER1)/wish.h
mdefault.o: $(HEADER1)/sizes.h
mdefault.o: $(HEADER1)/menu.h
mdefault.o: mdefault.c

mreshape.o: $(HEADER1)/ctl.h
mreshape.o: $(HEADER1)/menudefs.h
mreshape.o: $(HEADER1)/var_arrays.h
mreshape.o: $(HEADER1)/vtdefs.h
mreshape.o: $(HEADER1)/wish.h
mreshape.o: $(HEADER1)/menu.h
mreshape.o: mreshape.c

stmenu.o: $(HEADER1)/ctl.h
stmenu.o: $(HEADER1)/menudefs.h
stmenu.o: $(HEADER1)/message.h
stmenu.o: $(HEADER1)/moremacros.h
stmenu.o: $(HEADER1)/token.h
stmenu.o: $(HEADER1)/sizes.h
stmenu.o: $(HEADER1)/vtdefs.h
stmenu.o: $(HEADER1)/wish.h
stmenu.o: $(HEADER1)/menu.h
stmenu.o: stmenu.c

###### Standard makefile targets #######

all:		$(LIBRARY)

install:	all

clean:
		/bin/rm -f *.o

clobber:	clean
		/bin/rm -f $(LIBRARY)

.PRECIOUS:	$(LIBRARY)
