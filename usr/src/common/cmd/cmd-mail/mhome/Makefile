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

SUBDIRS	=	admin lib mailer

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
		if [ ! -d $(ETC)/mail/admin/px ] ; then mkdir -p $(ETC)/mail/admin/px ; fi
		if [ ! -d $(USRLIB) ] ; then mkdir -p $(USRLIB) ; fi
		if [ ! -d $(USR)/include ] ; then mkdir -p $(USR)/include ; fi
		# strip tcl programs here
		$(TCL) ../admin/lib/stripscript -c ../admin/lib/copyright admin/multihome
		$(TCL) ../admin/lib/stripscript -c ../admin/lib/copyright admin/vdomains
		$(INS) -f $(ETC)/mail/admin admin/multihome
		$(INS) -f $(ETC)/mail admin/vdomains

		$(INS) -f $(ETC)/mail mailer/multihome
		$(INS) -f $(USRLIB) lib/libmultihome.so
		$(INS) -f $(USR)/include lib/multihome.h
		$(INS) -f $(ETC)/mail/admin/px admin/px/domain.px
		$(INS) -f $(ETC)/mail/admin/px admin/px/baddomain.px

		if [ ! -d $(USRLIB)/scohelp/hooks ] ; then mkdir -p $(USRLIB)/scohelp/hooks ; fi
		$(INS) -f $(USRLIB)/scohelp/hooks admin/multihome.hk

		if [ ! -d $(ETC)/mail/admin/mhome.obj ] ; then mkdir -p $(ETC)/mail/admin/mhome.obj ; fi
		$(INS) -f $(ETC)/mail/admin/mhome.obj mhome.obj/activate.scoadmin
		$(DONLS) -d mhome.obj -o mhome install
		$(DONLS) -d admin/NLS install

local-install:
		if [ ! -d /etc/mail/admin/px ] ; then mkdir -p /etc/mail/admin/px ; fi
		if [ ! -d /usr/lib/scohelp/hooks ] ; then mkdir -p /usr/lib/scohelp/hooks ; fi

		cp admin/multihome /etc/mail/admin
		cp admin/vdomains /etc/mail
		chmod 755 /etc/mail/vdomains
		cp mailer/multihome /etc/mail
		cp lib/libmultihome.so /usr/lib
		cp lib/libmultihome.a /usr/lib
		cp lib/multihome.h /usr/include
		cp admin/px/* /etc/mail/admin/px
		cp admin/multihome.hk /usr/lib/scohelp/hooks
		-scoadmin -f -c mhome.obj
		$(DONLS) -d admin/NLS local-install
		$(DONLS) -d mhome.obj -o mhome local-install

local-remove:
		rm -fr /etc/mail/admin
		rm -f /usr/lib/libmultihome.so /usr/lib/libmultihome.a
		rm -f /usr/include/multihome.h
		rm -f /usr/lib/scohelp/hooks/multihome.hk
		scoadmin -d mhome.obj
		$(DONLS) -d admin/NLS local-remove
		$(DONLS) -d mhome.obj -o mhome local-remove


