#ident "@(#)Makefile	11.2"
#******************************************************************************
#
#	Copyright (C) 1993-1997 The Santa Cruz Operation, Inc.
#		All Rights Reserved.
#
#	The information in this file is provided for the exclusive use of
#	the licensees of The Santa Cruz Operation, Inc.  Such users have the
#	right to use, modify, and incorporate this code into other products
#	for purposes authorized by the license agreement provided they include
#	this notice and the associated copyright notice with any such product.
#	The information in this file is provided "AS IS" without warranty.
#
#===============================================================================

SHELL=/bin/sh

include		$(CMDRULES)

SUBDIRS	=	mag aliases channel domain

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
		if [ ! -d $(ETC)/mail/admin/px ] ; then mkdir -p $(ETC)/mail/admin/px ; fi
		# strip tcl programs here
		$(TCL) lib/stripscript -c lib/copyright mag/mailadmin
		$(INS) -f $(ETC)/mail/admin mag/mailadmin
		$(TCL) lib/stripscript -c lib/copyright channel/channel
		$(INS) -f $(ETC)/mail/admin channel/channel
		$(TCL) lib/stripscript -c lib/copyright domain/domain
		$(INS) -f $(ETC)/mail/admin domain/domain
		$(TCL) lib/stripscript -c lib/copyright aliases/aliases
		$(INS) -f $(ETC)/mail/admin aliases/aliases

		$(INS) -f $(ETC)/mail/admin/px mag/px/arrow.px
		$(INS) -f $(ETC)/mail/admin/px mag/px/blank.px
		$(INS) -f $(ETC)/mail/admin/px mag/px/folder_alone.px
		$(INS) -f $(ETC)/mail/admin/px mag/px/folder_has.px
		$(INS) -f $(ETC)/mail/admin/px mag/px/folder_next_alone.px
		$(INS) -f $(ETC)/mail/admin/px mag/px/folder_next_has.px
		$(INS) -f $(ETC)/mail/admin/px mag/px/folder_open_alone.px
		$(INS) -f $(ETC)/mail/admin/px mag/px/folder_open_has.px
		$(INS) -f $(ETC)/mail/admin/px mag/px/folder_open_next_alone.px
		$(INS) -f $(ETC)/mail/admin/px mag/px/folder_open_next_has.px
		$(INS) -f $(ETC)/mail/admin/px mag/px/hbar.px
		$(INS) -f $(ETC)/mail/admin/px mag/px/item_last.px
		$(INS) -f $(ETC)/mail/admin/px mag/px/item_middle.px
		$(INS) -f $(ETC)/mail/admin/px mag/px/next.px
		$(INS) -f $(ETC)/mail/admin/px mag/px/prev.px
		$(INS) -f $(ETC)/mail/admin/px mag/px/vbar.px

		
		if [ ! -d $(ETC)/mail/admin/mail.obj ] ; then mkdir -p $(ETC)/mail/admin/mail.obj ; fi
		$(INS) -f $(ETC)/mail/admin/mail.obj mail.obj/activate.scoadmin
		$(DONLS) -d aliases/NLS install
		$(DONLS) -d channel/NLS install
		$(DONLS) -d domain/NLS install
		$(DONLS) -d mag/NLS install
		$(DONLS) -d mail.obj -o mail install

		if [ ! -d $(USRLIB)/scohelp/hooks ] ; then mkdir -p $(USRLIB)/scohelp/hooks ; fi
		$(INS) -f $(USRLIB)/scohelp/hooks lib/aliases.hk
		$(INS) -f $(USRLIB)/scohelp/hooks lib/channel.hk
		$(INS) -f $(USRLIB)/scohelp/hooks lib/domain.hk
		$(INS) -f $(USRLIB)/scohelp/hooks lib/mailadmin.hk

local-install:
		if [ ! -d /etc/mail/admin/px ] ; then mkdir -p /etc/mail/admin/px ; fi
		if [ ! -d /etc/mail/table ] ; then mkdir -p /etc/mail/table ; fi
		if [ ! -d /usr/lib/scohelp/hooks ] ; then mkdir -p /usr/lib/scohelp/hooks ; fi
		cp mag/mailadmin /etc/mail/admin
		cp channel/channel /etc/mail/admin
		cp domain/domain /etc/mail/admin
		cp aliases/aliases /etc/mail/admin
		cp mag/px/* /etc/mail/admin/px
		cp lib/aliases.hk /usr/lib/scohelp/hooks
		cp lib/channel.hk /usr/lib/scohelp/hooks
		cp lib/domain.hk /usr/lib/scohelp/hooks
		cp lib/mailadmin.hk /usr/lib/scohelp/hooks
		-scoadmin -f -c mail.obj
		$(DONLS) -d aliases/NLS local-install
		$(DONLS) -d channel/NLS local-install
		$(DONLS) -d domain/NLS local-install
		$(DONLS) -d mag/NLS local-install
		$(DONLS) -d mail.obj -o mail local-install

local-remove:
		rm -fr /etc/mail/admin
		rm -f /usr/lib/scohelp/hooks/aliases.hk
		rm -f /usr/lib/scohelp/hooks/channel.hk
		rm -f /usr/lib/scohelp/hooks/domain.hk
		rm -f /usr/lib/scohelp/hooks/mailadmin.hk
		scoadmin -d mail.obj
		$(DONLS) -d aliases/NLS local-remove
		$(DONLS) -d channel/NLS local-remove
		$(DONLS) -d domain/NLS local-remove
		$(DONLS) -d mag/NLS local-remove
		$(DONLS) -d mail.obj -o mail local-remove
