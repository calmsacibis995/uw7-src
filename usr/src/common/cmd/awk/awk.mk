#	copyright	"%c%"

#ident	"@(#)awk:awk.mk	2.17.4.1"
#ident "$Header$"

#If building on a system that doesn't have the snprintf() library
#function, remove the "#" from the following line
#LOCALDEF=-DNO_SNPRINTF
include $(CMDRULES)

INSDIR = $(USRBIN)
OWN=bin
GRP=bin

YACCRM=-rm
FRC =
YFLAGS=-d
LDLIBS=-lm -ll -lw -lgen
LINTFLAGS=-pu -lm
REL = current
LIST = lp

OBJECTS = awk.g.o awk.lx.o b.o lib.o main.o parse.o proctab.o run.o tran.o
SOURCES = awk.g.y awk.h awk.lx.l b.c lib.c main.c parse.c proctab.c run.c tran.c

all:  awk

awk:	y.tab.h
	$(MAKE) -f awk.mk objects 
	$(CC) -o awk $(OBJECTS) $(LDFLAGS) $(LDLIBS) $(SHLIBS)

objects: $(OBJECTS)

awk.g.o: awk.g.c \
	awk.h \
	$(INC)/stdlib.h \
	$(INC)/pfmt.h \
	$(INC)/memory.h \
	$(INC)/values.h \
	previ.tab.h

b.o: b.c \
	awk.h \
	$(INC)/stdlib.h \
	$(INC)/ctype.h \
	$(INC)/stdio.h \
	y.tab.h \
	$(INC)/pfmt.h \
	$(INC)/sys/euc.h \
	previ.tab.h

lib.o: lib.c \
	$(INC)/stdio.h \
	$(INC)/ctype.h \
	$(INC)/errno.h $(INC)/sys/errno.h \
	$(INC)/string.h \
	awk.h \
	$(INC)/stdlib.h \
	y.tab.h \
	$(INC)/pfmt.h \
	previ.tab.h

main.o: main.c \
	$(INC)/stdio.h \
	$(INC)/ctype.h \
	$(INC)/signal.h $(INC)/sys/signal.h \
	$(INC)/pfmt.h \
	$(INC)/errno.h $(INC)/sys/errno.h \
	$(INC)/string.h \
	$(INC)/locale.h \
	$(INC)/sys/euc.h \
	$(INC)/getwidth.h \
	$(INC)/locale.h \
	awk.h \
	$(INC)/stdlib.h \
	y.tab.h \
	previ.tab.h

parse.o: parse.c \
	$(INC)/stdio.h \
	$(INC)/pfmt.h \
	awk.h \
	$(INC)/stdlib.h \
	y.tab.h \
	previ.tab.h

proctab.o: proctab.c \
	awk.h \
	$(INC)/stdlib.h \
	y.tab.h \
	previ.tab.h

proctab.c: maketab.c \
	previ.tab.h
	$(HCC) maketab.c -o maketab
	./maketab > proctab.c

run.o: run.c \
	awk.h \
	$(INC)/stdlib.h \
	$(INC)/math.h \
	y.tab.h \
	$(INC)/stdio.h \
	$(INC)/ctype.h \
	$(INC)/setjmp.h \
	$(INC)/pfmt.h \
	$(INC)/string.h \
	$(INC)/errno.h $(INC)/sys/errno.h \
	previ.tab.h

tran.o: tran.c \
	$(INC)/stdio.h \
	$(INC)/ctype.h \
	$(INC)/string.h \
	awk.h \
	$(INC)/stdlib.h \
	y.tab.h \
	$(INC)/pfmt.h \
	previ.tab.h

y.tab.h awk.g.c:	awk.g.y
		$(YACC) $(YFLAGS) awk.g.y
		mv y.tab.c awk.g.c

previ.tab.h:	y.tab.h
	-cmp -s y.tab.h prevy.tab.h || (cp y.tab.h prevy.tab.h; echo change maketab)

install: all
	$(INS) -f $(INSDIR) -m 0555 -u $(OWN) -g $(GRP) awk
	$(CH)-ln $(INSDIR)/awk $(INSDIR)/nawk

clean:
	-rm -f a.out *.o t.* *temp* *.out *junk* y.tab.* ./maketab proctab.c yacc.*

clobber:	clean
	-rm -f awk.g.c prevy.tab.*
	-rm  -f  awk nawk

lintit:
	$(LINT) $(LINTFLAGS) b.c main.c token.c tran.c run.c lib.c parse.c |\
		egrep -v '^(error|free|malloc)'

#	optional targets

src:	$(SOURCES) test.a awk.mk
	cp $? $(RDIR)
	touch src

get:
	for i in $(SOURCES) awk.mk; do cp $(RDIR)/$$i .; done

bin:
	cp a.out $(INSDIR)/awk
	strip $(INSDIR)/awk

profile:	$(OBJECTS) mon.o
	$(CC) -p -i $(OBJECTS) mon.o $(LIBS)

find:
	egrep -n "$(PAT)" *.[ylhc] awk.def

list:
	-pr WISH $(SOURCES) awk.mk tokenscript README EXPLAIN

diffs:
	-for i in $(SOURCES); do echo $$i:; diff $$i $(RDIR) | ind; done
lcomp:
	-rm -f  [b-z]*.o
	lcomp b.c main.c tran.c run.c lib.c parse.c proctab.c *.o $(LIBS)

FRC:

build:	bldmk
	get -p -r`gsid awk $(REL)` s.awk.src $(REWIRE) | ntar -d $(RDIR) -g
	cd $(RDIR) ; $(YACC) $(YFLAGS) awk.g.y
	cd $(RDIR) ; mv y.tab.c  awk.g.c ; rm -f y.tab.h

bldmk:
	get -p -r`gsid awk.km $(REL) s.awk.mk >$(RDIR)/awk.mk

listing:
	pr awk.mk $(SOURCES) | $(LIST)

delete:	clobber
	rm -f $(SOURCES)

