#ident	"@(#)fmli:qued/qued.mk	1.18.4.2"

include $(CMDRULES)

LIBRARY=libqued.a
HEADER1=../inc
LOCALINC=-I$(HEADER1)
OBJECTS= acs_io.o \
	arrows.o \
	copyfield.o \
	fclear.o \
	fgo.o \
	fstream.o \
	fput.o \
	fread.o \
	initfield.o \
	editmulti.o \
	editsingle.o \
	getfield.o \
	mfuncs.o \
	multiline.o \
	putfield.o \
	setfield.o \
	scrollbuf.o \
	sfuncs.o \
	singleline.o \
	vfuncs.o \
	wrap.o 

$(LIBRARY): $(OBJECTS)
	$(AR) $(ARFLAGS) $(LIBRARY) $(OBJECTS)

acs_io.o: $(HEADER1)/token.h
acs_io.o: $(HEADER1)/vt.h
acs_io.o: $(HEADER1)/vtdefs.h
acs_io.o: $(HEADER1)/winp.h
acs_io.o: $(HEADER1)/wish.h
acs_io.o: ./fmacs.h
acs_io.o: acs_io.c

arrows.o: $(HEADER1)/ctl.h
arrows.o: $(HEADER1)/vtdefs.h
arrows.o: $(HEADER1)/winp.h
arrows.o: $(HEADER1)/wish.h
arrows.o: ./fmacs.h
arrows.o: arrows.c

copyfield.o: $(HEADER1)/moremacros.h
copyfield.o: $(HEADER1)/terror.h
copyfield.o: $(HEADER1)/token.h
copyfield.o: $(HEADER1)/winp.h
copyfield.o: $(HEADER1)/wish.h
copyfield.o: ./fmacs.h
copyfield.o: copyfield.c

editmulti.o: $(HEADER1)/token.h
editmulti.o: $(HEADER1)/winp.h
editmulti.o: $(HEADER1)/wish.h
editmulti.o: ./fmacs.h
editmulti.o: editmulti.c

editsingle.o: $(HEADER1)/terror.h
editsingle.o: $(HEADER1)/token.h
editsingle.o: $(HEADER1)/winp.h
editsingle.o: $(HEADER1)/wish.h
editsingle.o: ./fmacs.h
editsingle.o: editsingle.c

fclear.o: $(HEADER1)/token.h
fclear.o: $(HEADER1)/winp.h
fclear.o: ./fmacs.h
fclear.o: fclear.c

fgo.o: $(HEADER1)/token.h
fgo.o: $(HEADER1)/winp.h
fgo.o: fgo.c

fput.o: $(HEADER1)/attrs.h
fput.o: $(HEADER1)/token.h
fput.o: $(HEADER1)/winp.h
fput.o: $(HEADER1)/wish.h
fput.o: ./fmacs.h
fput.o: fput.c

fread.o: $(HEADER1)/token.h
fread.o: $(HEADER1)/vt.h
fread.o: $(HEADER1)/vtdefs.h
fread.o: $(HEADER1)/winp.h
fread.o: $(HEADER1)/wish.h
fread.o: ./fmacs.h
fread.o: fread.c

fstream.o: $(HEADER1)/attrs.h
fstream.o: $(HEADER1)/ctl.h
fstream.o: $(HEADER1)/terror.h
fstream.o: $(HEADER1)/token.h
fstream.o: $(HEADER1)/vtdefs.h
fstream.o: $(HEADER1)/winp.h
fstream.o: $(HEADER1)/wish.h
fstream.o: ./fmacs.h
fstream.o: fstream.c

getfield.o: $(HEADER1)/terror.h
getfield.o: $(HEADER1)/token.h
getfield.o: $(HEADER1)/winp.h
getfield.o: $(HEADER1)/wish.h
getfield.o: ./fmacs.h
getfield.o: getfield.c

initfield.o: $(HEADER1)/eft.types.h
initfield.o: $(HEADER1)/inc.types.h
initfield.o: $(HEADER1)/attrs.h
initfield.o: $(HEADER1)/ctl.h
initfield.o: $(HEADER1)/terror.h
initfield.o: $(HEADER1)/token.h
initfield.o: $(HEADER1)/vtdefs.h
initfield.o: $(HEADER1)/winp.h
initfield.o: $(HEADER1)/wish.h
initfield.o: ./fmacs.h
initfield.o: initfield.c

mfuncs.o: $(HEADER1)/vt.h
mfuncs.o: $(HEADER1)/winp.h
mfuncs.o: $(HEADER1)/wish.h
mfuncs.o: ./fmacs.h
mfuncs.o: mfuncs.c

multiline.o: $(HEADER1)/token.h
multiline.o: $(HEADER1)/winp.h
multiline.o: $(HEADER1)/wish.h
multiline.o: ./fmacs.h
multiline.o: multiline.c

putfield.o: $(HEADER1)/moremacros.h
putfield.o: $(HEADER1)/terror.h
putfield.o: $(HEADER1)/token.h
putfield.o: $(HEADER1)/winp.h
putfield.o: $(HEADER1)/wish.h
putfield.o: ./fmacs.h
putfield.o: putfield.c

scrollbuf.o: $(HEADER1)/terror.h
scrollbuf.o: $(HEADER1)/token.h
scrollbuf.o: $(HEADER1)/winp.h
scrollbuf.o: $(HEADER1)/wish.h
scrollbuf.o: ./fmacs.h
scrollbuf.o: scrollbuf.c

setfield.o: $(HEADER1)/attrs.h
setfield.o: $(HEADER1)/terror.h
setfield.o: $(HEADER1)/token.h
setfield.o: $(HEADER1)/winp.h
setfield.o: $(HEADER1)/wish.h
setfield.o: ./fmacs.h
setfield.o: setfield.c

sfuncs.o: $(HEADER1)/attrs.h
sfuncs.o: $(HEADER1)/token.h
sfuncs.o: $(HEADER1)/vtdefs.h
sfuncs.o: $(HEADER1)/winp.h
sfuncs.o: $(HEADER1)/wish.h
sfuncs.o: ./fmacs.h
sfuncs.o: sfuncs.c

singleline.o: $(HEADER1)/token.h
singleline.o: $(HEADER1)/winp.h
singleline.o: $(HEADER1)/wish.h
singleline.o: ./fmacs.h
singleline.o: singleline.c

vfuncs.o: $(HEADER1)/ctl.h
vfuncs.o: $(HEADER1)/terror.h
vfuncs.o: $(HEADER1)/token.h
vfuncs.o: $(HEADER1)/vtdefs.h
vfuncs.o: $(HEADER1)/winp.h
vfuncs.o: $(HEADER1)/wish.h
vfuncs.o: ./fmacs.h
vfuncs.o: vfuncs.c

wrap.o: $(HEADER1)/terror.h
wrap.o: $(HEADER1)/token.h
wrap.o: $(HEADER1)/winp.h
wrap.o: $(HEADER1)/wish.h
wrap.o: ./fmacs.h
wrap.o: wrap.c

###### Standard makefile targets #####

all:		$(LIBRARY)

install:	all

clean:
		/bin/rm -f *.o

clobber:	clean
		/bin/rm -f $(LIBRARY)

.PRECIOUS:	libqued.a 
