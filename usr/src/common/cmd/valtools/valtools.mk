#ident	"@(#)valtools:valtools.mk	1.1.11.4"

include $(CMDRULES)

LDLIBS = -lpkg -ladm -lw -lgen

MSGS = valtools.str

OWN = bin
GRP = bin

#LLIBADM=$(USRLIB)/llib-ladm.ln
#LLIBPKG=$(USRLIB)/llib-lpkg.ln
#LINTLIBS=$(LLIBADM) $(LLIBPKG)

SOURCES = ckint.c ckitem.c ckpath.c ckrange.c ckstr.c ckyorn.c ckkeywd.c \
	puttext.c ckdate.c cktime.c ckuid.c ckgid.c

FILES = ckint ckitem ckpath ckrange ckstr ckyorn ckkeywd puttext ckdate \
	cktime ckuid ckgid

LOCALHEADS = $(INC)/stdio.h $(INC)/string.h \
	$(INC)/signal.h $(INC)/sys/signal.h \
	usage.h

all: $(FILES) $(MSGS)

ckint: ckint.c $(LOCALHEADS)
	$(CC) $(CFLAGS) $(DEFLIST) -o $@ $@.c $(LDFLAGS) $(LDLIBS) $(SHLIBS)

ckitem: ckitem.c \
	$(LOCALHEADS) \
	$(INC)/ctype.h \
	$(INC)/valtools.h
	$(CC) $(CFLAGS) $(DEFLIST) -o $@ $@.c $(LDFLAGS) $(LDLIBS) $(SHLIBS)

ckpath: ckpath.c \
	$(LOCALHEADS) \
	$(INC)/valtools.h 
	$(CC) $(CFLAGS) $(DEFLIST) -o $@ $@.c $(LDFLAGS) $(LDLIBS) $(SHLIBS)

ckrange: ckrange.c \
	$(LOCALHEADS) \
	$(INC)/limits.h \
	$(INC)/values.h
	$(CC) $(CFLAGS) $(DEFLIST) -o $@ $@.c $(LDFLAGS) $(LDLIBS) $(SHLIBS)

ckstr: ckstr.c $(LOCALHEADS)
	$(CC) $(CFLAGS) $(DEFLIST) -o $@ $@.c $(LDFLAGS) $(LDLIBS) $(SHLIBS)

ckyorn: ckyorn.c $(LOCALHEADS)
	$(CC) $(CFLAGS) $(DEFLIST) -o $@ $@.c $(LDFLAGS) $(LDLIBS) $(SHLIBS)

ckkeywd: ckkeywd.c $(LOCALHEADS)
	$(CC) $(CFLAGS) $(DEFLIST) -o $@ $@.c $(LDFLAGS) $(LDLIBS) $(SHLIBS)

puttext: puttext.c \
	$(LOCALHEADS) \
	$(INC)/ctype.h
	$(CC) $(CFLAGS) $(DEFLIST) -o $@ $@.c $(LDFLAGS) $(LDLIBS) $(SHLIBS)

ckdate: ckdate.c $(LOCALHEADS)
	$(CC) $(CFLAGS) $(DEFLIST) -o $@ $@.c $(LDFLAGS) $(LDLIBS) $(SHLIBS)

cktime: cktime.c $(LOCALHEADS)
	$(CC) $(CFLAGS) $(DEFLIST) -o $@ $@.c $(LDFLAGS) $(LDLIBS) $(SHLIBS)

ckuid: ckuid.c $(LOCALHEADS)
	$(CC) $(CFLAGS) $(DEFLIST) -o $@ $@.c $(LDFLAGS) $(LDLIBS) $(SHLIBS)

ckgid: ckgid.c $(LOCALHEADS)
	$(CC) $(CFLAGS) $(DEFLIST) -o $@ $@.c $(LDFLAGS) $(LDLIBS) $(SHLIBS)

clean:
	rm -f *.o

clobber: clean
	rm -f $(FILES)

lintit:
	rm -f lint.out
	for file in $(SOURCES) ;\
	do \
	echo '## lint output for '$$file' ##' >>lint.out ;\
	$(LINT) $(LINTFLAGS) $$file $(LINTLIBS) >>lint.out ;\
	done

install: all
	-[ -d $(USRLIB)/locale/C/MSGFILES ] || \
		mkdir -p $(USRLIB)/locale/C/MSGFILES
	$(INS) -f $(USRLIB)/locale/C/MSGFILES -m 644 valtools.str
	[ -d $(USRSADM)/bin ] || mkdir -p $(USRSADM)/bin
	$(CH)chmod 0755 $(USRSADM)
	$(CH)chgrp $(GRP) $(USRSADM)
	$(CH)chown $(OWN) $(USRSADM)
	$(INS) -f $(USRSADM)/bin -m 0555 -u $(OWN) -g $(GRP) puttext
	$(INS) -f $(USRBIN) -m 0555 -u $(OWN) -g $(GRP) ckint
	$(CH)-rm -f $(USRSADM)/bin/valint
	$(CH)-rm -f $(USRSADM)/bin/helpint
	$(CH)-rm -f $(USRSADM)/bin/errint
	$(CH)ln $(USRBIN)/ckint $(USRSADM)/bin/valint
	$(CH)ln $(USRBIN)/ckint $(USRSADM)/bin/helpint
	$(CH)ln $(USRBIN)/ckint $(USRSADM)/bin/errint
	$(INS) -f $(USRBIN) -m 0555 -u $(OWN) -g $(GRP) ckitem
	$(CH)-rm -f $(USRSADM)/bin/helpitem
	$(CH)-rm -f $(USRSADM)/bin/erritem
	$(CH)ln $(USRBIN)/ckitem $(USRSADM)/bin/helpitem
	$(CH)ln $(USRBIN)/ckitem $(USRSADM)/bin/erritem
	$(INS) -f $(USRBIN) -m 0555 -u $(OWN) -g $(GRP) ckpath
	$(CH)-rm -f $(USRSADM)/bin/valpath
	$(CH)-rm -f $(USRSADM)/bin/helppath
	$(CH)-rm -f $(USRSADM)/bin/errpath
	$(CH)ln $(USRBIN)/ckpath $(USRSADM)/bin/valpath
	$(CH)ln $(USRBIN)/ckpath $(USRSADM)/bin/helppath
	$(CH)ln $(USRBIN)/ckpath $(USRSADM)/bin/errpath
	$(INS) -f $(USRBIN) -m 0555 -u $(OWN) -g $(GRP) ckrange
	$(CH)-rm -f $(USRSADM)/bin/valrange
	$(CH)-rm -f $(USRSADM)/bin/helprange
	$(CH)-rm -f $(USRSADM)/bin/errange
	$(CH)ln $(USRBIN)/ckrange $(USRSADM)/bin/valrange
	$(CH)ln $(USRBIN)/ckrange $(USRSADM)/bin/helprange
	$(CH)ln $(USRBIN)/ckrange $(USRSADM)/bin/errange
	$(INS) -f $(USRBIN) -m 0555 -u $(OWN) -g $(GRP) ckstr
	$(CH)-rm -f $(USRSADM)/bin/valstr
	$(CH)-rm -f $(USRSADM)/bin/helpstr
	$(CH)-rm -f $(USRSADM)/bin/errstr
	$(CH)ln $(USRBIN)/ckstr $(USRSADM)/bin/valstr
	$(CH)ln $(USRBIN)/ckstr $(USRSADM)/bin/helpstr
	$(CH)ln $(USRBIN)/ckstr $(USRSADM)/bin/errstr
	$(INS) -f $(USRBIN) -m 0555 -u $(OWN) -g $(GRP) ckyorn
	$(CH)-rm -f $(USRSADM)/bin/valyorn
	$(CH)-rm -f $(USRSADM)/bin/helpyorn
	$(CH)-rm -f $(USRSADM)/bin/erryorn
	$(CH)ln $(USRBIN)/ckyorn $(USRSADM)/bin/valyorn
	$(CH)ln $(USRBIN)/ckyorn $(USRSADM)/bin/helpyorn
	$(CH)ln $(USRBIN)/ckyorn $(USRSADM)/bin/erryorn
	$(INS) -f $(USRBIN) -m 0555 -u $(OWN) -g $(GRP) ckkeywd
	$(INS) -f $(USRBIN) -m 0555 -u $(OWN) -g $(GRP) cktime
	$(CH)-rm -f $(USRSADM)/bin/valtime
	$(CH)-rm -f $(USRSADM)/bin/helptime
	$(CH)-rm -f $(USRSADM)/bin/errtime
	$(CH)ln $(USRBIN)/cktime $(USRSADM)/bin/valtime
	$(CH)ln $(USRBIN)/cktime $(USRSADM)/bin/helptime
	$(CH)ln $(USRBIN)/cktime $(USRSADM)/bin/errtime
	$(INS) -f $(USRBIN) -m 0555 -u $(OWN) -g $(GRP) ckdate
	$(CH)-rm -f $(USRSADM)/bin/valdate
	$(CH)-rm -f $(USRSADM)/bin/helpdate
	$(CH)-rm -f $(USRSADM)/bin/errdate
	$(CH)ln $(USRBIN)/ckdate $(USRSADM)/bin/valdate
	$(CH)ln $(USRBIN)/ckdate $(USRSADM)/bin/helpdate
	$(CH)ln $(USRBIN)/ckdate $(USRSADM)/bin/errdate
	$(INS) -f $(USRBIN) -m 0555 -u $(OWN) -g $(GRP) ckuid
	$(CH)-rm -f $(USRBIN)/dispuid
	$(CH)ln $(USRBIN)/ckuid $(USRBIN)/dispuid
	$(CH)-rm -f $(USRSADM)/bin/valuid
	$(CH)-rm -f $(USRSADM)/bin/helpuid
	$(CH)-rm -f $(USRSADM)/bin/erruid
	$(CH)-rm -f $(USRSADM)/bin/dispuid
	$(CH)ln $(USRBIN)/ckuid $(USRSADM)/bin/valuid
	$(CH)ln $(USRBIN)/ckuid $(USRSADM)/bin/helpuid
	$(CH)ln $(USRBIN)/ckuid $(USRSADM)/bin/erruid
	$(CH)ln $(USRBIN)/ckuid $(USRSADM)/bin/dispuid
	$(INS) -f $(USRBIN) -m 0555 -u $(OWN) -g $(GRP) ckgid
	$(CH)-rm -f $(USRBIN)/dispgid
	$(CH)ln $(USRBIN)/ckgid $(USRBIN)/dispgid
	$(CH)-rm -f $(USRSADM)/bin/valgid
	$(CH)-rm -f $(USRSADM)/bin/helpgid
	$(CH)-rm -f $(USRSADM)/bin/errgid
	$(CH)-rm -f $(USRSADM)/bin/dispgid
	$(CH)ln $(USRBIN)/ckgid $(USRSADM)/bin/valgid
	$(CH)ln $(USRBIN)/ckgid $(USRSADM)/bin/helpgid
	$(CH)ln $(USRBIN)/ckgid $(USRSADM)/bin/errgid
	$(CH)ln $(USRBIN)/ckgid $(USRSADM)/bin/dispgid
