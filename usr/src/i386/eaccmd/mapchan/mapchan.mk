#	copyright	"%c%"

#ident	"@(#)mapchan.mk	1.2"
#ident  "$Header$"

include $(CMDRULES)

#
#	Copyright (C) The Santa Cruz Operation, 1984-1988, 1989.
#	This Module contains Proprietary Information of
#	The Santa Cruz Operation, and should be treated as Confidential.

OWN = bin
GRP = bin

DIRS = $(USRBIN) $(ETC)/default $(USRLIB)/mapchan
DFTS = default
DATFILES = ascii deadcomp ibm iso dec nrc.can tvi.usa hp.roman8

.MUTEX: lex.yy.c

OBJECTS = lex.yy.o convert.o display.o oops.o deflt.o
MAINOBJECTS = mapchan.o trchan.o
LOCALDEF = -DM_I386

all: mapchan trchan

mapchan: mapchan.o $(OBJECTS)
	$(CC) -o mapchan mapchan.o $(OBJECTS) $(LDFLAGS) $(LDLIBS) $(SHLIBS)

trchan: trchan.o $(OBJECTS)
	$(CC) -o trchan trchan.o $(OBJECTS) $(LDFLAGS) $(LDLIBS) $(SHLIBS)

convert.o: convert.c \
	defs.h \
	$(INC)/stdio.h \
	$(INC)/sys/param.h \
	$(INC)/sys/emap.h \
	nmap.h \
	$(INC)/ctype.h

deflt.o: deflt.c \
	$(INC)/stdio.h

display.o: display.c \
	defs.h \
	$(INC)/stdio.h \
	$(INC)/sys/param.h \
	$(INC)/sys/emap.h \
	nmap.h \
	$(INC)/ctype.h

mapchan.o: mapchan.c \
	defs.h \
	$(INC)/stdio.h \
	$(INC)/sys/param.h \
	$(INC)/sys/emap.h \
	$(INC)/sys/termio.h \
	$(INC)/sys/emap.h \
	$(INC)/ctype.h \
	$(INC)/sys/types.h \
	$(INC)/sys/stat.h \
	$(INC)/fcntl.h \
	$(INC)/errno.h \
	$(INC)/signal.h \
	nmap.h

oops.o: oops.c \
	$(INC)/stdio.h

trchan.o: trchan.c \
	defs.h \
	$(INC)/stdio.h \
	$(INC)/sys/param.h \
	$(INC)/sys/emap.h

lex.yy.o: lex.yy.c

lex.yy.c: lex.l defs.h
	$(LEX) $(LFLAGS) lex.l

install: $(DIRS) all
	$(INS) -f $(USRBIN) -m 0711 -u $(OWN) -g $(GRP) trchan
	$(INS) -f $(USRBIN) -m 0711 -u $(OWN) -g $(GRP) mapchan
	-rm -f $(ETC)/default/mapchan
	-cp default $(ETC)/default/mapchan 
	-for i in $(DATFILES) ; do \
		rm -f $(USRLIB)/mapchan/$$i ;\
		if [ -f $$i ] ; then \
			cp $$i $(USRLIB)/mapchan/$$i ;\
			$(CH)chmod 644 $(USRLIB)/mapchan/$$i ;\
			$(CH)chgrp $(GRP) $(USRLIB)/mapchan/$$i ;\
			$(CH)chown $(OWN) $(USRLIB)/mapchan/$$i ;\
		fi ;\
	done 

$(DIRS):
	[ -d $@ ] || mkdir -p $@
		$(CH)chmod 0755 $@
		$(CH)chown $(OWN) $@
		$(CH)chgrp $(GRP) $@

clean:
	rm -f $(OBJECTS) $(MAINOBJECTS) lex.yy.c

clobber: clean
	rm -f mapchan trchan

lintit:
	$(LINT) $(LINTFLAGS) mapchan.c $(OBJECTS:.o=.c)
	$(LINT) $(LINTFLAGS) trchan.c $(OBJECTS:.o=.c)

