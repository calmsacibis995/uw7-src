#ident	"@(#)fmli:wish/wish.mk	1.40.4.2"

include $(CMDRULES)

LIBRARY=libwish.a
#LOCALDEF = -DRELEASE='"FMLI Release 4.0 Load K9"'
HEADER1=../inc
LOCALINC=-I$(HEADER1)
OBJECTS= browse.o \
	display.o \
	error.o \
	getstring.o \
	global.o \
	mudge.o \
	objop.o \
	virtual.o \
	wdwcreate.o \
	wdwlist.o \
	wdwmgmt.o

$(LIBRARY): $(OBJECTS)
	$(AR) $(ARFLAGS) $(LIBRARY) $(OBJECTS)

browse.o: $(HEADER1)/actrec.h
browse.o: $(HEADER1)/ctl.h
browse.o: $(HEADER1)/moremacros.h
browse.o: $(HEADER1)/slk.h
browse.o: $(HEADER1)/terror.h
browse.o: $(HEADER1)/token.h
browse.o: $(HEADER1)/wish.h
browse.o: browse.c

display.o: $(HEADER1)/eft.types.h
display.o: $(HEADER1)/inc.types.h
display.o: $(HEADER1)/message.h
display.o: $(HEADER1)/sizes.h
display.o: $(HEADER1)/typetab.h
display.o: $(HEADER1)/wish.h
display.o: display.c

error.o: $(HEADER1)/token.h
error.o: $(HEADER1)/wish.h
error.o: error.c

getstring.o: $(HEADER1)/actrec.h
getstring.o: $(HEADER1)/moremacros.h
getstring.o: $(HEADER1)/slk.h
getstring.o: $(HEADER1)/token.h
getstring.o: $(HEADER1)/vtdefs.h
getstring.o: $(HEADER1)/winp.h
getstring.o: $(HEADER1)/wish.h
getstring.o: getstring.c

global.o: $(HEADER1)/eft.types.h
global.o: $(HEADER1)/inc.types.h
global.o: $(HEADER1)/actrec.h
global.o: $(HEADER1)/ctl.h
global.o: $(HEADER1)/message.h
global.o: $(HEADER1)/moremacros.h
global.o: $(HEADER1)/slk.h
global.o: $(HEADER1)/terror.h
global.o: $(HEADER1)/token.h
global.o: $(HEADER1)/wish.h
global.o: global.c

mudge.o: $(HEADER1)/actrec.h
mudge.o: $(HEADER1)/ctl.h
mudge.o: $(HEADER1)/moremacros.h
mudge.o: $(HEADER1)/slk.h
mudge.o: $(HEADER1)/token.h
mudge.o: $(HEADER1)/vtdefs.h
mudge.o: $(HEADER1)/wish.h
mudge.o: mudge.c

objop.o: $(HEADER1)/eft.types.h
objop.o: $(HEADER1)/inc.types.h
objop.o: $(HEADER1)/message.h
objop.o: $(HEADER1)/moremacros.h
objop.o: $(HEADER1)/optabdefs.h
objop.o: $(HEADER1)/procdefs.h
objop.o: $(HEADER1)/terror.h
objop.o: $(HEADER1)/typetab.h
objop.o: $(HEADER1)/wish.h
objop.o: $(HEADER1)/sizes.h
objop.o: objop.c

virtual.o: $(HEADER1)/actrec.h
virtual.o: $(HEADER1)/message.h
virtual.o: $(HEADER1)/moremacros.h
virtual.o: $(HEADER1)/slk.h
virtual.o: $(HEADER1)/token.h
virtual.o: $(HEADER1)/vtdefs.h
virtual.o: $(HEADER1)/wish.h
virtual.o: virtual.c

wdwcreate.o: $(HEADER1)/actrec.h
wdwcreate.o: $(HEADER1)/ctl.h
wdwcreate.o: $(HEADER1)/menudefs.h
wdwcreate.o: $(HEADER1)/message.h
wdwcreate.o: $(HEADER1)/slk.h
wdwcreate.o: $(HEADER1)/terror.h
wdwcreate.o: $(HEADER1)/token.h
wdwcreate.o: $(HEADER1)/vtdefs.h
wdwcreate.o: $(HEADER1)/wish.h
wdwcreate.o: $(HEADER1)/sizes.h
wdwcreate.o: wdwcreate.c

wdwlist.o: $(HEADER1)/actrec.h
wdwlist.o: $(HEADER1)/ctl.h
wdwlist.o: $(HEADER1)/helptext.h
wdwlist.o: $(HEADER1)/menudefs.h
wdwlist.o: $(HEADER1)/moremacros.h
wdwlist.o: $(HEADER1)/slk.h
wdwlist.o: $(HEADER1)/terror.h
wdwlist.o: $(HEADER1)/token.h
wdwlist.o: $(HEADER1)/vtdefs.h
wdwlist.o: $(HEADER1)/wish.h
wdwlist.o: wdwlist.c

wdwmgmt.o: $(HEADER1)/actrec.h
wdwmgmt.o: $(HEADER1)/ctl.h
wdwmgmt.o: $(HEADER1)/helptext.h
wdwmgmt.o: $(HEADER1)/menudefs.h
wdwmgmt.o: $(HEADER1)/moremacros.h
wdwmgmt.o: $(HEADER1)/slk.h
wdwmgmt.o: $(HEADER1)/terror.h
wdwmgmt.o: $(HEADER1)/token.h
wdwmgmt.o: $(HEADER1)/vtdefs.h
wdwmgmt.o: $(HEADER1)/wish.h
wdwmgmt.o: wdwmgmt.c

###### Standard makefile targets #######

all:		$(LIBRARY)

install:	all

clean:
		/bin/rm -f *.o

clobber:	clean
		/bin/rm -f $(LIBRARY)

.PRECIOUS:	$(LIBRARY)
