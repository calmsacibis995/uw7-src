#
#	ident @(#) Makefile 11.3 97/11/13 
#
#############################################################################
#
#	Copyright (c) 1997 The Santa Cruz Operation, Inc.. 
#		All Rights Reserved. 
#
#	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF 
#		THE SANTA CRUZ OPERATION INC.
#	The copyright notice above does not evidence any
#	actual or intended publication of such source code.
#
#############################################################################
#
#
# Modification History
#
# M001, 08-Sep-97, andrean
#	- us97-24535: Added install targets for new message catalogs.
#	- Also, new copyright.
#
# M000, 15-Apr-97, andrean
#	- Created.
#

SHELL=/bin/sh

include		$(CMDRULES)

SUBDIRS	=	mfck vacation

TCL =		$(TOOLS)/usr/bin/tcl
KSH =		/bin/ksh
MKCATDECL =	$(TCL) $(TOOLS)/usr/bin/mkcatdecl
MKCATDEFS =	$(TOOLS)/usr/bin/mkcatdefs
DONLS =		TCL=$(TCL) \
		INS=$(INS) \
		MKCATDECL="$(MKCATDECL)" \
		MKCATDEFS=$(MKCATDEFS) \
		USRLIB=$(USRLIB) ETC=$(ETC) \
		$(KSH) ../bin/donls

#
# Default targets.
#
all:		build

#
# Run make in all our subdirectories.
#
build clean clobber:
	@for dir in $(SUBDIRS); \
	do \
	    if [ ! -d $$dir ]; \
	    then \
		continue; \
	    fi; \
	    ( \
		echo "------- '$(MAKE) -$(MAKEFLAGS) $@' in $${dir}"; \
		cd $$dir; \
		$(MAKE) -$(MAKEFLAGS) $@ \
	    ); \
	done

install:	build
	        if [ ! -d $(USRBIN) ] ; then mkdir -p $(USRBIN) ; fi
	        if [ ! -d $(ETC)/mail/admin/dt/C ] ; then mkdir -p $(ETC)/mail/admin/dt/C ; fi
	        if [ ! -d $(ETC)/mail/admin/dt/de ] ; then mkdir -p $(ETC)/mail/admin/dt/de ; fi
	        if [ ! -d $(ETC)/mail/admin/dt/es ] ; then mkdir -p $(ETC)/mail/admin/dt/es ; fi
	        if [ ! -d $(ETC)/mail/admin/dt/fr ] ; then mkdir -p $(ETC)/mail/admin/dt/fr ; fi
	        if [ ! -d $(ETC)/mail/admin/dt/ja ] ; then mkdir -p $(ETC)/mail/admin/dt/ja ; fi
		if [ ! -d $(USRLIB)/scohelp/hooks ] ; then mkdir -p $(USRLIB)/scohelp/hooks ; fi
		# strip tcl programs here
		$(TCL) ../admin/lib/stripscript -c ../admin/lib/copyright vacation/vacation
		$(INS) -f $(ETC)/mail/admin vacation/vacation
		$(INS) -f $(USRBIN) mfck/mfck
		$(INS) -f $(ETC)/mail/admin/dt/C vacation/dt/C/vacation.dt
		$(INS) -f $(ETC)/mail/admin/dt/C vacation/dt/C/vacation.fp
		$(INS) -f $(ETC)/mail/admin/dt/de vacation/dt/de/vacation.fp
		$(INS) -f $(ETC)/mail/admin/dt/es vacation/dt/es/vacation.fp
		$(INS) -f $(ETC)/mail/admin/dt/fr vacation/dt/fr/vacation.fp
		$(INS) -f $(ETC)/mail/admin/dt/ja vacation/dt/ja/vacation.fp
		$(INS) -f $(USRLIB)/scohelp/hooks vacation/vacation.hk
		$(DONLS) -d mfck/NLS install
		$(DONLS) -d vacation/NLS install

local-install:
		if [ ! -d /etc/mail/admin/dt/C ] ; then mkdir -p /etc/mail/admin/dt/C ; fi
		if [ ! -d /etc/mail/admin/dt/de ] ; then mkdir -p /etc/mail/admin/dt/de ; fi
		if [ ! -d /etc/mail/admin/dt/es ] ; then mkdir -p /etc/mail/admin/dt/es ; fi
		if [ ! -d /etc/mail/admin/dt/fr ] ; then mkdir -p /etc/mail/admin/dt/fr ; fi
		if [ ! -d /etc/mail/admin/dt/ja ] ; then mkdir -p /etc/mail/admin/dt/ja ; fi
		if [ ! -d /usr/lib/scohelp/hooks ] ; then mkdir -p /usr/lib/scohelp/hooks ; fi
		cp vacation/vacation /etc/mail/admin
		cp vacation/dt/C/vacation.dt /etc/mail/admin/dt/C
		cp vacation/dt/C/vacation.fp /etc/mail/admin/dt/C
		cp vacation/dt/de/vacation.fp /etc/mail/admin/dt/de
		cp vacation/dt/es/vacation.fp /etc/mail/admin/dt/es
		cp vacation/dt/fr/vacation.fp /etc/mail/admin/dt/fr
		cp vacation/dt/ja/vacation.fp /etc/mail/admin/dt/ja
		cp vacation/vacation.hk /usr/lib/scohelp/hooks
		cp mfck/mfck /usr/bin
		$(DONLS) -d mfck/NLS local-install
		$(DONLS) -d vacation/NLS local-install

local-remove:
		rm -fr /etc/mail/admin/vacation /etc/mail/admin/dt /usr/bin/mfck
		rm -f /usr/lib/scohelp/hooks/vacation.hk
		$(DONLS) -d mfck/NLS local-remove
		$(DONLS) -d vacation/NLS local-remove
