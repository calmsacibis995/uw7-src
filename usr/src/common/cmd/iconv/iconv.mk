#ident	"@(#)iconv.mk	1.2"

include $(CMDRULES)

LOCALINC= -I.

ICONV=$(USRLIB)/iconv
LICONV=/usr/lib/iconv
OWN = bin
GRP = bin

IOBJS=	iconv.o
KOBJS=	main.o gram.o lexan.o output.o reach.o sort.o sym.o tree.o

MAINS = iconv kbdcomp

CODESETS=\
	codesets/646da.8859.p codesets/646de.8859.p codesets/646en.8859.p \
	codesets/646es.8859.p codesets/646fr.8859.p codesets/646it.8859.p \
	codesets/646sv.8859.p codesets/8859.646.p codesets/8859.646da.p \
	codesets/8859.646de.p codesets/8859.646en.p codesets/8859.646es.p \
	codesets/8859.646fr.p codesets/8859.646it.p codesets/8859.646sv.p \
	codesets/8859-1.dk.p codesets/Case.p codesets/Cmacs.p codesets/Deutsche.p \
	codesets/Dvorak.p codesets/PFkeytest.p codesets/lnktst.p

.MUTEX: gram.c

all:	$(MAINS)

iconv:	$(IOBJS)
	$(CC) -o $@ $(IOBJS) $(LDLIBS) $(SHLIBS) $(LDFLAGS)

kbdcomp: $(KOBJS)
	$(CC) -o $@ $(KOBJS) $(LDLIBS) $(SHLIBS) $(LDFLAGS)

install : $(MAINS)
	$(INS) -f $(USRBIN) -m 0555 -u $(OWN) -g $(GRP) iconv
	$(INS) -f $(USRBIN) -m 0555 -u $(OWN) -g $(GRP) kbdcomp
	- [ -d $(ICONV)/codesets ] || mkdir -p $(ICONV)/codesets ; \
		$(CH)chmod 755 $(ICONV)/codesets ; \
		$(CH)chown $(OWN) $(ICONV)/codesets ; \
		$(CH)chgrp $(GRP) $(ICONV)/codesets
	$(INS) -f  $(ICONV) -m 0444 -u $(OWN) -g $(GRP) codesets/iconv_data
	for i in  $(CODESETS) ;\
	do \
		# $(CH)./local_kbdcomp -o `basename $$i .p` $$i ; \
		$(CH)./kbdcomp -o `basename $$i .p` $$i ; \
		$(CH)$(INS) -f $(ICONV) -m 0444 -u $(OWN) -g $(GRP) `basename $$i .p` ; \
		$(INS) -f $(ICONV)/codesets -m 0444 -u $(OWN) -g $(GRP) $$i; \
	done

	
$(IOBJS): ./symtab.h ./kbd.h

$(KOBJS):	./symtab.h ./kbd.h

.PRECIOUS:	gram.y

gram.c:	gram.y ./symtab.h ./kbd.h
	$(YACC) -vd gram.y
	mv y.tab.c gram.c

.c.o:
	$(CC) $(CFLAGS) $(DEFLIST) -c $*.c

clean:
	rm -f *.o *.t y.tab.h y.output gram.c 
	
clobber: clean
	rm -f *.o *.t iconv kbdcomp
	$(CH)for i in  $(CODESETS) ;\
	$(CH)do \
		$(CH) rm -f `basename $$i .p` ; \
	$(CH)done

lintit:
	$(LINT) $(LINTFLAGS) $(IOBJS:.o=.c)
	$(LINT) $(LINTFLAGS) $(KOBJS:.o=.c)
