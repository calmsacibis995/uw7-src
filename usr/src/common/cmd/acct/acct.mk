#	copyright	"%c%"

#ident	"@(#)acct:common/cmd/acct/acct.mk	1.9.5.14"
#ident "$Header$"

include $(CMDRULES)


FRC =
CONFIGDIR = $(ETC)/acct
ADMDIR = $(VAR)/adm
ETCINIT = $(ETC)/init.d
WKDIR = nite fiscal sum
LIB = lib/a.a
INSDIR=$(USRLIB)/acct
SRCDIR = .

LDLIBS=$(LIB) -lgen

SOURCES= acctcms.c acctcom.c acctcon.c acctcon1.c acctcon2.c acctdisk.c \
	acctdusg.c acctmerg.c accton.c acctprc.c acctprc1.c acctprc2.c \
	acctwtmp.c closewtmp.c diskusg.c bfsdiskusg.c ufsdiskusg.c sfsdiskusg.c\
	fwtmp.c wtmpfix.c utmp2wtmp.c

PRODUCTS = acctcms acctcom acctcon acctcon1 acctcon2 \
	acctdisk acctdusg acctmerg accton acctprc acctprc1 acctprc2 \
	acctwtmp closewtmp diskusg bfsdiskusg ufsdiskusg sfsdiskusg \
	fwtmp wtmpfix utmp2wtmp acct chargefee ckpacct dodisk \
	lastlogin monacct nulladm prctmp prdaily prtacct \
	remove runacct shutacct startup turnacct holtable \
	awkecms awkelus

.MUTEX:	install
.MUTEX:	library $(PRODUCTS)

all: cmds 
	
cmds:	library $(PRODUCTS)

library:
	cd lib; $(MAKE) -f Makefile $(MAKEARGS)

holtable: holidays $(FRC)

awkecms: ptecms.awk $(FRC)

awkelus: ptelus.awk $(FRC)

$(INSDIR):
	[ -d $@ ] || mkdir $@ ;\
	$(CH)chmod 775 $@ ;\
	$(CH)chown bin $@ ;\
	$(CH)chgrp bin $@

wkdirs:
	[ -d $(ADMDIR)/acct ] || mkdir -p $(ADMDIR)/acct ;\
		$(CH)chmod 775 $(ADMDIR)/acct ;\
		$(CH)chown adm $(ADMDIR)/acct ;\
		$(CH)chgrp adm $(ADMDIR)/acct ;\
	for dir in $(WKDIR) ;\
	do \
		[ -d $@ ] || mkdir -p $@ ;\
			$(CH)chmod 775 $@ ;\
			$(CH)chown adm $@ ;\
			$(CH)chgrp adm $@ ;\
	done

$(CONFIGDIR):
	[ -d $@ ] || mkdir $@ ;\
	$(CH)chmod 775 $@ ;\
	$(CH)chown adm $@ ;\
	$(CH)chgrp adm $@

install: all $(INSDIR) $(CONFIGDIR) wkdirs
	$(INS) -f $(INSDIR) acctcms
	$(INS) -f $(USRBIN) acctcom
	$(INS) -f $(INSDIR) acctcon
	$(INS) -f $(INSDIR) acctcon1
	$(INS) -f $(INSDIR) acctcon2
	$(INS) -f $(INSDIR) acctdisk
	$(INS) -f $(INSDIR) acctdusg
	$(INS) -f $(INSDIR) acctmerg
	$(INS) -f $(INSDIR) -u root -g adm -m 4755 accton
	$(INS) -f $(INSDIR) acctprc
	$(INS) -f $(INSDIR) acctprc1
	$(INS) -f $(INSDIR) acctprc2
	$(INS) -f $(INSDIR) acctwtmp
	$(INS) -f $(INSDIR) closewtmp
	$(INS) -f $(INSDIR) fwtmp
	$(INS) -f $(INSDIR) diskusg
	$(INS) -f $(INSDIR) bfsdiskusg
	$(INS) -f $(INSDIR) ufsdiskusg
	$(INS) -f $(INSDIR) sfsdiskusg
	$(INS) -f $(INSDIR) utmp2wtmp
	$(INS) -f $(INSDIR) wtmpfix
	$(INS) -f $(ETCINIT) -u root -g sys -m 444 acct
	$(INS) -f $(INSDIR) chargefee
	$(INS) -f $(INSDIR) ckpacct
	$(INS) -f $(INSDIR) dodisk
	$(INS) -f $(INSDIR) monacct
	$(INS) -f $(INSDIR) lastlogin
	$(INS) -f $(INSDIR) nulladm
	$(INS) -f $(INSDIR) prctmp
	$(INS) -f $(INSDIR) prdaily
	$(INS) -f $(INSDIR) prtacct
	$(INS) -f $(INSDIR) remove
	$(INS) -f $(INSDIR) runacct
	$(INS) -f $(INSDIR) shutacct
	$(INS) -f $(INSDIR) startup
	$(INS) -f $(INSDIR) turnacct
	$(INS) -f $(CONFIGDIR) -m 664 holidays
	$(INS) -f $(INSDIR) ptecms.awk
	$(INS) -f $(INSDIR) ptelus.awk

clean:
	-rm -f *.o
	cd lib; $(MAKE) clean $(MAKEARGS)

clobber: clean
	-rm -f $(PRODUCTS)
	cd lib; $(MAKE) clobber $(MAKEARGS)

lintit:
	$(LINT) $(LINTFLAGS) $(SOURCES)
	cd lib; $(MAKE) lintit $(MAKEARGS)

FRC:

.c: $(LIB) $< $(FRC)
	$(CC) $(CFLAGS) $(DEFLIST) -o $@ $< $(LDFLAGS) $(LDLIBS) $(SHLIBS)

.sh: $(SRCDIR)/$< $(FRC)
	cp $(SRCDIR)/$< $@
