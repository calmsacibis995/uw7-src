#ident "@(#)Makefile	11.2"

SHELL=/bin/sh

include		Make.defs

include		$(CMDRULES)

SUBDIRS		= execmail mailstats makemap praliases rmail scocf smrsh src

KSH		= /bin/ksh
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
		if [ ! -d $(ETC)/mail ] ; then mkdir -p $(ETC)/mail ; fi
		if [ ! -d $(BIN) ] ; then mkdir -p $(BIN) ; fi
		if [ ! -d $(USRLIB) ] ; then mkdir -p $(USRLIB) ; fi
		if [ ! -d $(USRLIB)/mail ] ; then mkdir -p $(USRLIB)/mail ; fi
		if [ ! -d $(USR)/mmdf ] ; then mkdir -p $(USR)/mmdf ; fi
		copy -r cf $(ETC)/mail/cf
		$(INS) -f $(USRLIB) src/sendmail
		$(INS) -f $(ETC)/mail src/sendmail.hf
		$(INS) -f $(ETC)/mail mailstats/mailstats
		$(INS) -f $(ETC)/mail makemap/makemap
		$(INS) -f $(ETC)/mail praliases/praliases
		$(INS) -f $(ETC)/mail smrsh/smrsh
		$(INS) -f $(BIN) rmail/rmail
		$(INS) -f $(ETC) scocf/sendmail.cf
		$(INS) -f $(ETC)/mail scocf/sendmailcf.factory
		$(INS) -f $(ETC)/mail scocf/sendmailcf.factory
		$(INS) -f $(ETC)/mail scocf/aliases
		$(INS) -f $(ETC)/mail scocf/sendmailrc
		$(INS) -f $(USRLIB)/mail execmail/execmail
		cp scocf/mmdftailor /tmp/mmdftailor-
		$(INS) -f $(USR)/mmdf /tmp/mmdftailor-
		rm -f /tmp/mmdftailor-
		$(INS) -f $(ETC)/mail scocf/socket
		$(DONLS) -d scocf/NLS install

local-install:
		if [ ! -d /etc/mail ] ; then mkdir -p /etc/mail ; fi
		if [ ! -d /var/spool/mqueue ] ; then mkdir -p /var/spool/mqueue ; fi
		chmod 700 /var/spool/mqueue
		if [ ! -d /usr/lib/mail ] ; then mkdir -p /usr/lib/mail ; fi
		if [ ! -d /var/mail ] ; then mkdir -p /var/mail ; fi
		if [ ! -d /usr/mmdf ] ; then mkdir -p /usr/mmdf ; fi
		copy -r cf /etc/mail/cf
		chmod 1777 /var/mail
		rm -f /etc/mail/smtpd /etc/mail/newaliases /etc/mail/mailq
		cp src/sendmail			/usr/lib/sendmail
		ln /usr/lib/sendmail		/etc/mail/sendmail
		ln /usr/lib/sendmail		/etc/mail/newaliases
		ln /usr/lib/sendmail		/etc/mail/mailq
		ln /usr/lib/sendmail		/etc/mail/smtpd
		ln /usr/lib/sendmail		/etc/mail/hoststat
		ln /usr/lib/sendmail		/etc/mail/purgestat
		chown root:mail /usr/lib/sendmail
		chmod 4755 /usr/lib/sendmail
		cp src/sendmail.hf		/etc/mail
		cp mailstats/mailstats		/etc/mail
		cp makemap/makemap		/etc/mail
		cp praliases/praliases		/etc/mail
		cp smrsh/smrsh			/etc/mail
		cp rmail/rmail			/usr/bin
		cp scocf/sendmail.cf		/etc
		cp scocf/sendmailcf.factory	/etc/mail
		cp scocf/aliases		/etc/mail
		cp execmail/execmail		/usr/lib/mail
		cp scocf/mmdftailor		/usr/mmdf/mmdftailor-
		cp scocf/sendmailrc		/etc/mail
		chmod 755 /etc/mail/sendmailrc
		ln /etc/mail/sendmailrc /etc/rc2.d/S81sendmail
		ln /etc/mail/sendmailrc /etc/rc1.d/K68sendmail
		ln /etc/mail/sendmailrc /etc/rc0.d/K68sendmail
		(cd /etc/mail; newaliases)
		cp scocf/socket /etc/mail
		$(DONLS) -d scocf/NLS local-install

local-remove:
		rm -fr /etc/mail
		rm -fr /etc/sendmail.cf
		rm -fr /var/mail
		rm -fr /var/spool/mqueue
		rm -fr /usr/lib/sendmail
		rm -fr /usr/bin/rmail
		rm -fr /usr/lib/mail/execmail
		rm -fr /usr/mmdf
		rm -f /etc/rc2.d/S81sendmail
		rm -f /etc/rc1.d/K68sendmail
		rm -f /etc/rc0.d/K68sendmail
		-rmdir /usr/lib/mail
		$(DONLS) -d scocf/NLS local-remove
