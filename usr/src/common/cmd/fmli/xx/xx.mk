#	copyright	"%c%"

#	Copyright (c) 1984 AT&T
#	  All Rights Reserved

#
#ident	"@(#)fmli:xx/xx.mk	1.39.4.3"
#

include $(CMDRULES)

LIBWISH	=	../wish/libwish.a
LIBVT	=	../vt/libvt.a
LIBFORM	=	../form/libform.a
LIBMENU	=	../menu/libmenu.a
LIBQUED	=	../qued/libqued.a
LIBOH	=	../oh/liboh.a
LIBOEU	=	../oeu/liboeu.a
LIBPROC	=	../proc/libproc.a
LIBSYS	=	../sys/libsys.a
#LIBGEN  = 	../gen/libgen.a

INSDIR = $(USRBIN)
OWN = bin
GRP = bin
HEADER1=../inc
LOCALINC=-I$(HEADER1)

#LIBS = $(LIBWISH) $(LIBOH) $(LIBOEU) $(LIBFORM) $(LIBQUED) $(LIBMENU) $(LIBPROC) $(LIBVT) $(LIBSYS) $(LIBCURSES) $(LIBGEN)

LIBS = $(LIBWISH) $(LIBOH) $(LIBOEU) $(LIBFORM) $(LIBQUED) $(LIBMENU) $(LIBPROC) $(LIBVT) $(LIBSYS)  

LDLIBS=$(LIBS) $(EXTRA_LIBS) -lcurses -lgen

BCMDS =	fmli vsig 

CMDS = $(BCMDS) 

all:	$(CMDS)

fmli: main.o $(LIBS)
	$(CC) $(LDFLAGS) -o $@ main.o $(LDLIBS) $(SHLIBS)

main.o: $(HEADER1)/eft.types.h
main.o: $(HEADER1)/inc.types.h
main.o: $(HEADER1)/actrec.h
main.o: $(HEADER1)/ctl.h
main.o: $(HEADER1)/moremacros.h
main.o: $(HEADER1)/sizes.h
main.o: $(HEADER1)/terror.h
main.o: $(HEADER1)/token.h
main.o: $(HEADER1)/vtdefs.h
main.o: $(HEADER1)/wish.h
main.o: main.c

vsig: vsig.o
	$(CC) $(LDFLAGS) -o $@ vsig.o $(SHLIBS)

vsig.o: $(HEADER1)/sizes.h
vsig.o: vsig.c

###### Standard Makefile Targets ######

install: all
	@set +e;\
	if [ ! -d $(INSDIR) ] ;\
	then \
		mkdir -p $(INSDIR) ;\
	fi
	for f in $(BCMDS);\
	do\
		$(INS) -f $(INSDIR) -m 0755 -u $(OWN) -g $(GRP) $$f;\
	done

clean: 
	@set +e;\
	for f in $(BCMDS);\
	do\
		/bin/rm -f $$f;\
	done;\
	/bin/rm -f *.o

clobber:	clean
