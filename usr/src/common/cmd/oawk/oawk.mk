#ident	"@(#)oawk:oawk.mk	1.8.3.1"

include $(CMDRULES)


OWN = bin
GRP = bin

RDIR = $(ROOT)/usr/src/common/cmd/oawk

YACCRM = -rm
YFLAGS = -d
IFLAGS = -i
LINTFLAGS = -pu
LDLIBS = -lm

REL = current
LIST = lp

OBJECTS = awk.g.o awk.lx.o b.o main.o token.o tran.o lib.o run.o parse.o \
	proctab.o freeze.o
SOURCES = EXPLAIN README awk.def awk.g.y awk.h awk.lx.l b.c lib.c main.c \
	parse.c freeze.c run.c tokenscript tran.c

all: y.tab.h
	$(MAKE) -f oawk.mk $(MAKEARGS) oawk

oawk: $(OBJECTS)
	$(CC) $(OBJECTS) -o oawk $(LDFLAGS) $(LDLIBS) $(SHLIBS)

awk.g.o: awk.g.y

awk.lx.o: awk.lx.l \
	awk.def \
	awk.h

b.o: b.c \
	awk.def \
	$(INC)/stdio.h \
	awk.h

freeze.o: freeze.c \
	$(INC)/stdio.h

lib.o: lib.c \
	$(INC)/stdio.h \
	awk.def \
	awk.h \
	$(INC)/ctype.h

main.o: main.c \
	$(INC)/stdio.h \
	$(INC)/ctype.h \
	awk.def \
	awk.h

parse.o: parse.c \
	awk.def \
	awk.h \
	$(INC)/stdio.h

proc.o: proc.c \
	awk.h

proctab.o: proctab.c

run.o: run.c \
	awk.def \
	$(INC)/math.h \
	awk.h \
	$(INC)/stdio.h

token.o: token.c \
	awk.h
	$(CC) $(CFLAGS) $(DEFLIST) -c token.c
	mv token.c.link token.c

tran.o: tran.c \
	$(INC)/stdio.h \
	awk.def \
	awk.h

awk.h: awk.g.o
	-cp y.tab.h awk.h

y.tab.h: awk.g.o

proctab.c: ./makeprctab
	./makeprctab >proctab.c

token.c: awk.h tokenscript
	mv token.c token.c.link
	cp token.c.link  token.c
	-chmod u+w token.c
	ed -s <tokenscript

#Compile makeprctab.c and token.c in the native environment. This is
#necessary to ensure that AWK will compile under a cross-compilation
#environment.

./makeprctab: makeprctab.o htoken.o
	$(HCC) -o ./makeprctab makeprctab.o htoken.o

htoken.o: token.c
	cp token.c htoken.c
	$(HCC) -O -c htoken.c

makeprctab.o: makeprctab.c
	$(HCC) -O -c makeprctab.c

install: all
	-rm -f $(USRBIN)/oawk
	 $(INS) -f $(USRBIN) -m 0555 -u $(OWN) -g $(GRP) oawk

clean:
	-rm -f a.out *.o t.* *temp* *.out *junk* y.tab.* awk.h ./makeprctab \
		awk.g.c htoken.c proctab.c

clobber: clean
	-rm -f oawk

lintit:
	$(LINT) $(LINTFLAGS) b.c main.c token.c tran.c run.c lib.c parse.c \
		$(LDLIBS) | egrep -v '^(error|free|malloc)'

# optional targets

src: $(SOURCES) test.a tokenscript makefile README
	cp $? $(RDIR)
	touch src

get:
	for i in $(SOURCES) oawk.mk tokenscript README; do cp $(RDIR)/$$i .; done

bin:
	cp a.out $(USRBIN)/oawk
	strip $(USRBIN)/oawk

profile: $(OBJECTS) mon.o
	$(CC) -p -i $(OBJECTS) mon.o $(LDLIBS)

find:
	egrep -n "$(PAT)" *.[ylhc] awk.def

list:
	-pr WISH $(SOURCES) oawk.mk tokenscript README EXPLAIN

diffs:
	-for i in $(SOURCES); do echo $$i:; diff $$i $(RDIR) | ind; done

lcomp:
	-rm -f [b-z]*.o
	lcomp b.c main.c token.c tran.c run.c lib.c parse.c freeze.c proctab.c  *.o -lm

build: bldmk
	get -p -r`gsid oawk $(REL)` s.awk.src $(REWIRE) | ntar -d $(RDIR) -g
	cd $(RDIR) ; $(YACC) $(YFLAGS) awk.g.y
	cd $(RDIR) ; mv y.tab.c awk.g.c ; rm -f y.tab.h

bldmk:
	get -p -r`gsid oawk.km $(REL) s.oawk.mk >$(RDIR)/oawk.mk

listing:
	pr oawk.mk $(SOURCES) | $(LIST)

delete: clobber
	rm -f $(SOURCES)

