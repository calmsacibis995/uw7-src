#ident	"@(#)fmli:vt/vt.mk	1.23.4.3"

include $(CMDRULES)

LIBRARY=libvt.a
HEADER1=../inc
CURSES_H=$(INC)
LOCALINC=-I$(HEADER1)
OBJECTS= fits.o \
	hide.o \
	highlight.o \
	indicator.o \
	makebox.o \
	message.o \
	move.o \
	offscreen.o \
	physical.o \
	redraw.o \
	vclose.o \
	vcolor.o \
	vcreate.o \
	vctl.o \
	vcurrent.o \
	vflush.o \
	vfork.o \
	vinit.o \
	vmark.o \
	vreshape.o \
	wclrwin.o \
	wdelchar.o \
	wgetchar.o \
	wgo.o \
	winschar.o \
	wputchar.o \
	wputs.o \
	wreadchar.o \
	wscrollwin.o \
	showmail.o \
	working.o 

$(LIBRARY): $(OBJECTS)
	$(AR) $(ARFLAGS) $(LIBRARY) $(OBJECTS)

fits.o: $(CURSES_H)/curses.h
fits.o: $(HEADER1)/vtdefs.h
fits.o: $(HEADER1)/wish.h
fits.o: fits.c

hide.o: $(CURSES_H)/curses.h
hide.o: $(HEADER1)/color_pair.h
hide.o: $(HEADER1)/vt.h
hide.o: $(HEADER1)/vtdefs.h
hide.o: $(HEADER1)/wish.h
hide.o: hide.c

highlight.o: $(CURSES_H)/curses.h
highlight.o: $(HEADER1)/color_pair.h
highlight.o: highlight.c

indicator.o: $(CURSES_H)/curses.h
indicator.o: $(HEADER1)/vt.h
indicator.o: $(HEADER1)/vtdefs.h
indicator.o: $(HEADER1)/wish.h
indicator.o: indicator.c

makebox.o: $(CURSES_H)/curses.h
makebox.o: $(HEADER1)/attrs.h
makebox.o: $(HEADER1)/vt.h
makebox.o: $(HEADER1)/vtdefs.h
makebox.o: $(HEADER1)/wish.h
makebox.o: makebox.c

message.o: $(CURSES_H)/curses.h
message.o: $(HEADER1)/message.h
message.o: $(HEADER1)/vt.h
message.o: $(HEADER1)/vtdefs.h
message.o: $(HEADER1)/wish.h
message.o: message.c

move.o: $(CURSES_H)/curses.h
move.o: $(HEADER1)/vt.h
move.o: $(HEADER1)/vtdefs.h
move.o: $(HEADER1)/wish.h
move.o: move.c

offscreen.o: $(CURSES_H)/curses.h
offscreen.o: $(HEADER1)/wish.h
offscreen.o: offscreen.c

physical.o: $(HEADER1)/eft.types.h
physical.o: $(HEADER1)/inc.types.h
physical.o: $(CURSES_H)/curses.h
physical.o: $(HEADER1)/actrec.h
physical.o: $(HEADER1)/message.h
physical.o: $(HEADER1)/moremacros.h
physical.o: $(HEADER1)/token.h
physical.o: $(HEADER1)/var_arrays.h
physical.o: $(HEADER1)/vt.h
physical.o: $(HEADER1)/vtdefs.h
physical.o: $(HEADER1)/wish.h
physical.o: physical.c

redraw.o: $(CURSES_H)/curses.h
redraw.o: $(HEADER1)/vt.h
redraw.o: $(HEADER1)/vtdefs.h
redraw.o: $(HEADER1)/wish.h
redraw.o: redraw.c

showmail.o: $(HEADER1)/eft.types.h
showmail.o: $(HEADER1)/inc.types.h
showmail.o: $(CURSES_H)/curses.h
showmail.o: $(HEADER1)/vt.h
showmail.o: $(HEADER1)/vtdefs.h
showmail.o: $(HEADER1)/wish.h
showmail.o: showmail.c

vclose.o: $(CURSES_H)/curses.h
vclose.o: $(HEADER1)/var_arrays.h
vclose.o: $(HEADER1)/vt.h
vclose.o: $(HEADER1)/vtdefs.h
vclose.o: $(HEADER1)/wish.h
vclose.o: vclose.c

vcolor.o: $(CURSES_H)/curses.h
vcolor.o: $(HEADER1)/color_pair.h
vcolor.o: $(HEADER1)/moremacros.h
vcolor.o: $(HEADER1)/vt.h
vcolor.o: $(HEADER1)/vtdefs.h
vcolor.o: $(HEADER1)/wish.h
vcolor.o: vcolor.c

vcreate.o: $(CURSES_H)/curses.h
vcreate.o: $(HEADER1)/color_pair.h
vcreate.o: $(HEADER1)/moremacros.h
vcreate.o: $(HEADER1)/var_arrays.h
vcreate.o: $(HEADER1)/vt.h
vcreate.o: $(HEADER1)/vtdefs.h
vcreate.o: $(HEADER1)/wish.h
vcreate.o: vcreate.c

vctl.o: $(CURSES_H)/curses.h
vctl.o: $(HEADER1)/attrs.h
vctl.o: $(HEADER1)/color_pair.h
vctl.o: $(HEADER1)/ctl.h
vctl.o: $(HEADER1)/vt.h
vctl.o: $(HEADER1)/vtdefs.h
vctl.o: $(HEADER1)/wish.h
vctl.o: vctl.c

vcurrent.o: $(CURSES_H)/curses.h
vcurrent.o: $(HEADER1)/color_pair.h
vcurrent.o: $(HEADER1)/vt.h
vcurrent.o: $(HEADER1)/vtdefs.h
vcurrent.o: $(HEADER1)/wish.h
vcurrent.o: vcurrent.c

vflush.o: $(CURSES_H)/curses.h
vflush.o: $(HEADER1)/attrs.h
vflush.o: $(HEADER1)/color_pair.h
vflush.o: $(HEADER1)/vt.h
vflush.o: $(HEADER1)/vtdefs.h
vflush.o: $(HEADER1)/wish.h
vflush.o: vflush.c

vfork.o: $(CURSES_H)/curses.h
vfork.o: $(HEADER1)/wish.h
vfork.o: vfork.c

vinit.o: $(CURSES_H)/curses.h
vinit.o: $(HEADER1)/attrs.h
vinit.o: $(HEADER1)/ctl.h
vinit.o: $(HEADER1)/token.h
vinit.o: $(HEADER1)/var_arrays.h
vinit.o: $(HEADER1)/vt.h
vinit.o: $(HEADER1)/vtdefs.h
vinit.o: $(HEADER1)/wish.h
vinit.o: vinit.c

vmark.o: $(CURSES_H)/curses.h
vmark.o: $(HEADER1)/vt.h
vmark.o: $(HEADER1)/vtdefs.h
vmark.o: $(HEADER1)/wish.h
vmark.o: vmark.c

vreshape.o: $(CURSES_H)/curses.h
vreshape.o: $(HEADER1)/color_pair.h
vreshape.o: $(HEADER1)/vt.h
vreshape.o: $(HEADER1)/vtdefs.h
vreshape.o: $(HEADER1)/wish.h
vreshape.o: vreshape.c

wclrwin.o: $(CURSES_H)/curses.h
wclrwin.o: $(HEADER1)/attrs.h
wclrwin.o: $(HEADER1)/vt.h
wclrwin.o: $(HEADER1)/vtdefs.h
wclrwin.o: $(HEADER1)/wish.h
wclrwin.o: wclrwin.c

wdelchar.o: $(CURSES_H)/curses.h
wdelchar.o: $(HEADER1)/vt.h
wdelchar.o: $(HEADER1)/vtdefs.h
wdelchar.o: $(HEADER1)/wish.h
wdelchar.o: wdelchar.c

wgetchar.o: $(CURSES_H)/curses.h
wgetchar.o: $(HEADER1)/token.h
wgetchar.o: $(HEADER1)/vt.h
wgetchar.o: $(HEADER1)/vtdefs.h
wgetchar.o: $(HEADER1)/wish.h
wgetchar.o: wgetchar.c

wgo.o: $(CURSES_H)/curses.h
wgo.o: $(HEADER1)/vt.h
wgo.o: $(HEADER1)/vtdefs.h
wgo.o: $(HEADER1)/wish.h
wgo.o: wgo.c

winschar.o: $(CURSES_H)/curses.h
winschar.o: $(HEADER1)/vt.h
winschar.o: $(HEADER1)/vtdefs.h
winschar.o: $(HEADER1)/wish.h
winschar.o: winschar.c

working.o: $(CURSES_H)/curses.h
working.o: $(HEADER1)/vt.h
working.o: $(HEADER1)/vtdefs.h
working.o: $(HEADER1)/wish.h
working.o: working.c

wputchar.o: $(CURSES_H)/curses.h
wputchar.o: $(HEADER1)/vt.h
wputchar.o: $(HEADER1)/vtdefs.h
wputchar.o: $(HEADER1)/wish.h
wputchar.o: wputchar.c

wputs.o: $(CURSES_H)/curses.h
wputs.o: $(HEADER1)/attrs.h
wputs.o: $(HEADER1)/vt.h
wputs.o: $(HEADER1)/wish.h
wputs.o: wputs.c

wreadchar.o: $(CURSES_H)/curses.h
wreadchar.o: $(HEADER1)/vt.h
wreadchar.o: $(HEADER1)/vtdefs.h
wreadchar.o: $(HEADER1)/wish.h
wreadchar.o: wreadchar.c

wscrollwin.o: $(CURSES_H)/curses.h
wscrollwin.o: $(HEADER1)/attrs.h
wscrollwin.o: $(HEADER1)/vt.h
wscrollwin.o: $(HEADER1)/vtdefs.h
wscrollwin.o: $(HEADER1)/wish.h
wscrollwin.o: wscrollwin.c

###### Standard makefile targets #######

all:		$(LIBRARY)

install:	all

clean:
		/bin/rm -f *.o

clobber:	clean
		/bin/rm -f $(LIBRARY)

.PRECIOUS:	$(LIBRARY)
