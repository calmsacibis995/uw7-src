#ident	"@(#)cscope:i386/makefile	1.9"

include $(CMDRULES)
PROGRAM = cscope
COMPAT_PROGRAM = compat_cscope
#
INS=../../install/install.sh
INSDIR=$(CCSBIN)

COMCS= ../common
INCLIST= -I$(COMCS) -I$(INC)
SGSINC=../../sgs/inc/i386
COMINC=../../sgs/inc/common
DEFLIST= -DCCS
#
LIBS= -lcurses -ll -lgen

OBJ = dir.o crossref.o scanner.o lookup.o command.o display.o find.o \
	 edit.o exec.o help.o history.o input.o


LIBOBJ = alloc.o basename.o compath.o egrep.o invlib.o logdir.o \
	 mouse.o mygetenv.o mypopen.o vpaccess.o vpfopen.o vpinit.o vpopen.o

OBJECTS = main.o $(OBJ) $(LIBOBJ)
COMPAT_OBJECTS = compat_main.o $(OBJ) $(LIBOBJ)

CFILES =  $(COMCS)/main.c $(COMCS)/dir.c $(COMCS)/crossref.c \
	scanner.c $(COMCS)/lookup.c $(COMCS)/command.c \
	$(COMCS)/display.c $(COMCS)/find.c $(COMCS)/edit.c \
	$(COMCS)/exec.c $(COMCS)/help.c $(COMCS)/history.c $(COMCS)/input.c \
	$(COMCS)/mouse.c $(COMCS)/alloc.c $(COMCS)/basename.c \
	$(COMCS)/compath.c egrep.c $(COMCS)/invlib.c \
	$(COMCS)/logdir.c $(COMCS)/mygetenv.c $(COMCS)/mypopen.c \
	$(COMCS)/vpaccess.c $(COMCS)/vpfopen.c $(COMCS)/vpinit.c \
	$(COMCS)/vpopen.c

all:	$(PROGRAM) $(COMPAT_PROGRAM)

$(PROGRAM): $(OBJECTS)
	$(CC) $(LDFLAGS) -o $@ $(OBJECTS) $(LINK_MODE) $(LIBS)

$(COMPAT_PROGRAM): $(COMPAT_OBJECTS)
	$(CC) $(LDFLAGS) -o $@ $(COMPAT_OBJECTS) $(LINK_MODE) $(LIBS)

main.o:	$(COMCS)/constants.h $(COMCS)/version.h $(COMCS)/main.c
	$(CC) -c $(CFLAGS) $(DEFLIST) $(INCLIST) $(COMCS)/main.c

compat_main.o:	$(COMCS)/constants.h $(COMCS)/version.h $(COMCS)/main.c
	$(CC) -c $(CFLAGS) -Wa,"-ocompat_main.o" \
	-DDFLT_INCDIR=\"$(ALT_PREFIX)/usr/include\" $(DEFLIST) $(INCLIST) $(COMCS)/main.c

dir.o:	$(COMCS)/constants.h $(COMCS)/vp.h $(COMCS)/dir.c
	$(CC) -c $(CFLAGS) $(DEFLIST) $(INCLIST) $(COMCS)/dir.c
	
crossref.o:	$(COMCS)/constants.h $(COMCS)/invlib.h $(COMCS)/crossref.c
	$(CC) -c $(CFLAGS) $(DEFLIST) $(INCLIST) $(COMCS)/crossref.c

scanner.c:	$(COMCS)/constants.h $(COMCS)/scanner.l
	$(LEX) -n -t $(COMCS)/scanner.l >scanner.c

scanner.o:	scanner.c $(COMCS)/global.h
	$(CC) -c $(CFLAGS) $(DEFLIST) $(INCLIST) scanner.c

lookup.o:	$(COMCS)/constants.h $(COMCS)/lookup.c
	$(CC) -c $(CFLAGS) $(DEFLIST) $(INCLIST) $(COMCS)/lookup.c

command.o:	$(COMCS)/constants.h $(COMCS)/command.c
	$(CC) -c $(CFLAGS) $(DEFLIST) $(INCLIST) $(COMCS)/command.c

display.o:	$(COMCS)/constants.h $(COMCS)/version.h $(COMCS)/display.c
	$(CC) -c $(CFLAGS) $(DEFLIST) $(INCLIST) -I$(SGSINC) $(COMCS)/display.c
		
find.o:	$(COMCS)/constants.h $(COMCS)/invlib.h $(COMCS)/find.c
	$(CC) -c $(CFLAGS) $(DEFLIST) $(INCLIST) $(COMCS)/find.c

edit.o: $(COMCS)/constants.h $(COMCS)/edit.c
	$(CC) -c $(CFLAGS) $(DEFLIST) $(INCLIST) $(COMCS)/edit.c

exec.o:	$(COMCS)/constants.h $(COMCS)/exec.c
	$(CC) -c $(CFLAGS) $(DEFLIST) $(INCLIST) $(COMCS)/exec.c

help.o:	$(COMCS)/constants.h $(COMCS)/help.c
	$(CC) -c $(CFLAGS) $(DEFLIST) $(INCLIST) $(COMCS)/help.c

history.o:	$(COMCS)/constants.h $(COMCS)/history.c
	$(CC) -c $(CFLAGS) $(DEFLIST) $(INCLIST) $(COMCS)/history.c

input.o: $(COMCS)/constants.h $(COMCS)/input.c
	$(CC) -c $(CFLAGS) $(DEFLIST) $(INCLIST) $(COMCS)/input.c

invlib.o: $(COMCS)/invlib.h $(COMCS)/invlib.c
	$(CC) -c $(CFLAGS) $(DEFLIST) $(INCLIST) $(COMCS)/invlib.c

mouse.o: $(COMCS)/constants.h $(COMCS)/mouse.c
	$(CC) -c $(CFLAGS) $(DEFLIST) $(INCLIST) $(COMCS)/mouse.c

alloc.o:	$(COMCS)/alloc.c
	$(CC) -c $(CFLAGS) $(DEFLIST) $(INCLIST) $(COMCS)/alloc.c

basename.o:	$(COMCS)/basename.c
	$(CC) -c $(CFLAGS) $(DEFLIST) $(INCLIST) $(COMCS)/basename.c

compath.o:	$(COMCS)/compath.c
	$(CC) -c $(CFLAGS) $(DEFLIST) $(INCLIST) $(COMCS)/compath.c

egrep.c:	$(COMCS)/egrep.y
	$(YACC) $(COMCS)/egrep.y
	mv y.tab.c egrep.c
egrep.o:	egrep.c
	$(CC) -c $(CFLAGS) $(DEFLIST) $(INCLIST) egrep.c

logdir.o:	$(COMCS)/logdir.c
	$(CC) -c $(CFLAGS) $(DEFLIST) $(INCLIST) $(COMCS)/logdir.c

mygetenv.o:	$(COMCS)/mygetenv.c
	$(CC) -c $(CFLAGS) $(DEFLIST) $(INCLIST) $(COMCS)/mygetenv.c

mypopen.o:	$(COMCS)/mypopen.c
	$(CC) -c $(CFLAGS) $(DEFLIST) $(INCLIST) $(COMCS)/mypopen.c

vpaccess.o:	$(COMCS)/vp.h $(COMCS)/vpaccess.c
	$(CC) -c $(CFLAGS) $(DEFLIST) $(INCLIST) -I$(COMINC) $(COMCS)/vpaccess.c

vpfopen.o:	$(COMCS)/vp.h $(COMCS)/vpfopen.c
	$(CC) -c $(CFLAGS) $(DEFLIST) $(INCLIST) $(COMCS)/vpfopen.c

vpinit.o:	$(COMCS)/vp.h $(COMCS)/vpinit.c
	$(CC) -c $(CFLAGS) $(DEFLIST) $(INCLIST) $(COMCS)/vpinit.c

vpopen.o:	$(COMCS)/vp.h $(COMCS)/vpopen.c
	$(CC) -c $(CFLAGS) $(DEFLIST) $(INCLIST) $(COMCS)/vpopen.c

install: all
	cp $(PROGRAM) $(PROGRAM).bak
	$(STRIP) $(PROGRAM)
	/bin/sh $(INS) -f $(INSDIR) $(PROGRAM)
	cp $(COMPAT_PROGRAM) $(COMPAT_PROGRAM).bak
	$(STRIP) $(COMPAT_PROGRAM)
	mv $(COMPAT_PROGRAM) $(PROGRAM)
	/bin/sh $(INS) -f $(UW_CCSBIN) $(PROGRAM)
	/bin/sh $(INS) -f $(OSR5_CCSBIN) $(PROGRAM)
	mv $(COMPAT_PROGRAM).bak $(COMPAT_PROGRAM)
	mv $(PROGRAM).bak $(PROGRAM)
clean:
	rm -f *.o *.out lex.yy.c y.tab.? scanner.c egrep.c

clobber: clean
	rm -f $(PROGRAM) $(COMPAT_PROGRAM)

strip:	$(PROGRAM)
	-$(STRIP) $(PROGRAM)

lintit:	$(CFILES)
	$(LINT) $(LINTFLAGS) $(INCLIST) $(DEFLIST) $(ODEFLIST) $(LIBS) $(CFILES)
