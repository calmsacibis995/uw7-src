#ident	"@(#)Makefile	11.1"

#	Copyright (c) 1990, 1991, 1992, 1993 Novell, Inc. All Rights Reserved.
#	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 Novell, Inc. All Rights Reserved.
#	  All Rights Reserved

#	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.
#	The copyright notice above does not evidence any
#	actual or intended publication of such source code.

SHELL=/bin/sh

CFLAGS = -I.
LDLIBS=
INS= install
USRBIN= /usr/bin
USRLIB= /usr/lib
ETC= /etc

include $(CMDRULES)

KSH =		/bin/ksh
TCL =		$(TOOLS)/usr/bin/tcl
MKCATDECL =	$(TCL) $(TOOLS)/usr/bin/mkcatdecl
MKCATDEFS =	$(TOOLS)/usr/bin/mkcatdefs
DONLS =		TCL=$(TCL) \
		INS=$(INS) \
		MKCATDECL="$(MKCATDECL)" \
		MKCATDEFS=$(MKCATDEFS) \
		USRLIB=$(USRLIB) ETC=$(ETC) \
		$(KSH) ../bin/donls

#
# This is where config.h lives
CONFIGDIR = .

# these are the programs created here
BINPROGS1= extcompose metasend
BINPROGS2= getfilename showaudio showexternal shownonascii showpartial showpicture
BINPROGS= $(BINPROGS1) $(BINPROGS2)
METAPROGS= mailto metamail mimencode splitmail
RICHPROGS= richtext



#### build targets ####

all: all-metamail all-richmail all-bin metamail.str

build:	all

all-metamail:
	cd metamail && $(MAKE) CONFIGDIR=../${CONFIGDIR}

all-richmail:
	cd richmail && $(MAKE) CONFIGDIR=../${CONFIGDIR} richtext

all-bin:
	cd bin && $(MAKE) CONFIGDIR=../${CONFIGDIR}

metamail.str: pfmt.msgs
	@if [ -r metamail.str ]; then rm -f metamail.str; fi
	sed	-e 's/^:[0-9][0-9]*://' \
		-e 's/^:U_[0-9][0-9]*://' \
		-e '/^#ident.*attmail/d' < pfmt.msgs > metamail.str



#### install targets ####

install: install-usr-lib install-mailcap install-bin install-metamail install-richmail install-messages

install-usr-lib: $(USRLIB)/mail/metamail

$(USRLIB)/mail/metamail:
	if [ ! -d $(USRLIB)/mail/metamail ]; \
	then \
		mkdir -p $(USRLIB)/mail/metamail ; \
	fi

install-mailcap: mailcap
	if [ ! -d $(ETC) ]; \
	then \
		mkdir -p $(ETC); \
	fi
	$(INS) -f $(ETC) mailcap

install-bin:
	if [ ! -d $(USRBIN) ]; \
	then \
		mkdir -p $(USRBIN) ; \
	fi
	if [ ! -d $(USRLIB)/mail/metamail ]; \
	then \
		mkdir -p $(USRLIB)/mail/metamail ; \
	fi
	cd bin && for i in $(BINPROGS1); do $(INS) -f $(USRBIN) $$i; done
	cd bin && for i in $(BINPROGS2); do $(INS) -f $(USRLIB)/mail/metamail $$i; done
	$(DONLS) -d bin/NLS install

install-metamail: all-metamail
	cd metamail && for i in $(METAPROGS); do $(INS) -f $(USRBIN) $$i; done

install-richmail: all-richmail
	cd richmail && for i in $(RICHPROGS); do $(INS) -f $(USRLIB)/mail/metamail $$i; done


install-messages: metamail.str
	if [ ! -d $(USRLIB)/locale/C/MSGFILES ]; \
	then \
		mkdir -p $(USRLIB)/locale/C/MSGFILES ; \
	fi
	$(INS) -f $(USRLIB)/locale/C/MSGFILES metamail.str

#### clean targets ####

clean:
	(cd metamail; $(MAKE) -$(MAKEFLAGS) clean)
	(cd richmail; $(MAKE) -$(MAKEFLAGS) clean)
	(cd bin; $(MAKE) -$(MAKEFLAGS) clean)

#### clobber targets ####

clobber: clean
	(cd metamail; $(MAKE) -$(MAKEFLAGS) clobber)
	(cd richmail; $(MAKE) -$(MAKEFLAGS) clobber)
	(cd bin; $(MAKE) -$(MAKEFLAGS) clobber)
	rm -f metamail.str


#### lintit targets

lintit: lint-metamail lint-richmail

lint-metamail:
	# cd metamail && $(LINT) ??

lint-richmail:
	# cd richmail && $(LINT) ??



#### strip targets ####

strip: strip-bin strip-metamail strip-richmail

strip-bin:

strip-metamail:
	cd metamail && strip $(METAPROGS)

strip-richmail:
	cd richmail && strip $(RICHPROGS)


local-install:
	if [ ! -d /etc/mail ] ; then mkdir -p /etc/mail ; fi
	if [ ! -d /usr/lib/LOCALE/C/MSGFILES ] ; then mkdir -p /usr/lib/LOCALE/C/MSGFILES ; fi
	if [ ! -d /usr/lib/mail/metamail ] ; then mkdir -p /usr/lib/mail/metamail ; fi

	cp mailcap /etc
	cp bin/extcompose /usr/bin
	cp bin/metasend /usr/bin
	cp metamail/mailto /usr/bin
	cp metamail/metamail /usr/bin
	cp metamail/mimencode /usr/bin
	cp metamail/splitmail /usr/bin
	cp metamail.str /usr/lib/LOCALE/C/MSGFILES
	cp bin/getfilename /usr/lib/mail/metamail
	cp bin/showaudio /usr/lib/mail/metamail
	cp bin/showexternal /usr/lib/mail/metamail
	cp bin/shownonascii /usr/lib/mail/metamail
	cp bin/showpartial /usr/lib/mail/metamail
	cp bin/showpicture /usr/lib/mail/metamail
	cp richmail/richtext /usr/lib/mail/metamail
	$(DONLS) -d bin/NLS local-install

local-remove:
	rm -f /etc/mailcap
	rm -f /usr/bin/extcompose
	rm -f /usr/bin/metasend
	rm -f /usr/bin/mailto
	rm -f /usr/bin/metamail
	rm -f /usr/bin/mimencode
	rm -f /usr/bin/splitmail
	rm -f /usr/lib/LOCALE/C/MSGFILES/metamail.str
	rm -fr /usr/lib/mail/metamail
	-rmdir /usr/lib/mail
	$(DONLS) -d bin/NLS local-remove
