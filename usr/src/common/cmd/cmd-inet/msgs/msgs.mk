#      @(#)msgs.mk	1.3 STREAMWare for Unixware 2.0  source
#
# Copyrighted as an unpublished work.
# (c) Copyright 1987-1994 Lachman Technology, Inc.
# All rights reserved.
#

#ident	"@(#)msgs.mk	1.3"

include $(CMDRULES)

MSGS =	ppp.str pppconf.str rcp.str rlogin.str ping.str telnet.str rsh.str ftp.str bootp.str
TXT  =  txtstrings

HELPFILES = help/net.broad.hcf help/net.dnsname.hcf \
	help/net.dnsserv.hcf help/net.netmask.hcf help/net.routeIP.hcf \
	help/net.sysIP.hcf help/net.frame.hcf help/kbhelp.hcf \
	help/genhelp.hcf 

MSGINET  = $(USRLIB)/locale/C/MSGFILES
MENUINET = $(ETC)/inst/locale/C/menus/inet
HELPINET = $(MENUINET)/help
DIRS     = $(HELPINET) $(MSGINET)

all: $(MSGS) $(HELPFILES) $(TXT)

install: $(DIRS) all
	for i in $(MSGS);\
	do\
		$(INS) -f $(MSGINET) -m 644 $$i;\
	done
	$(INS) -f $(MENUINET) -m 644 $(TXT)
	for i in $(HELPFILES);\
	do\
		$(INS) -f $(HELPINET) -m 0444 $$i;\
	done

$(HELPFILES): $(@:.hcf=)
	$(USRBIN)/hcomp $(@:.hcf=)

$(DIRS):
	[ -d $@ ] || mkdir -p $@

clean:

lintit:

clobber:
	rm -f $(HELPFILES)
