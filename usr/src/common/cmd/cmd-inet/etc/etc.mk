#ident	"@(#)etc.mk	1.9"

#
#	STREAMware TCP
#	Copyright 1987, 1993 Lachman Technology, Inc.
#	All Rights Reserved.
#

include $(CMDRULES)

OWN=		root
GRP=		sys
INET=		$(ETC)/inet
SINET=		/etc/inet
CONFNET=	$(ETC)/confnet.d
INETCONF=	$(CONFNET)/inet
DIRS = 		$(INET) $(CONFNET) $(INETCONF)

MKDIRS=		init.d

FILES=		hosts inetd.conf networks protocols \
		services shells strcf

all:		$(FILES) inet/rc.inet inet/inet.priv \
			inet/listen.setup inet/menu inet/rc.restart \
			inet/named.boot.samp inet/if.ignore \
			inet/ppphosts.samp inet/pppauth.samp inet/config \
			inet/inet.dfl inet/gateways \
			confnet.d/inet/interface confnet.d/inet/config.boot.sh \
			confnet.d/inet/configure addrpool.samp inet/pppfilter.samp
		@for i in $(MKDIRS);\
		do\
			cd $$i;\
			/bin/echo "\n===== $(MAKE) -f $$i.mk all";\
			$(MAKE) -f $$i.mk all $(MAKEARGS);\
			cd ..;\
		done;\
		wait


install:	$(DIRS) all
		for i in $(FILES);\
		do\
			$(INS) -f $(INET) -m 0444 -u $(OWN) -g $(GRP) $$i;\
			rm -f $(ETC)/$$i;\
			$(SYMLINK) $(SINET)/$$i $(ETC)/$$i;\
		done
		$(INS) -f $(INET) -m 0540 -u $(OWN) -g $(GRP) inet/inet.priv
		$(INS) -f $(INET) -m 0444 -u $(OWN) -g $(GRP) inet/rc.inet
		$(INS) -f $(INET) -m 0755 -u $(OWN) -g $(GRP) inet/listen.setup
		$(INS) -f $(INET) -m 0755 -u $(OWN) -g $(GRP) inet/menu
		$(INS) -f $(INET) -m 0755 -u $(OWN) -g $(GRP) inet/rc.restart
		$(INS) -f $(INET) -m 0755 -u $(OWN) -g $(GRP) inet/named.boot.samp
		$(INS) -f $(INET) -m 0755 -u $(OWN) -g $(GRP) inet/if.ignore
		$(INS) -f $(INET) -m 0755 -u $(OWN) -g $(GRP) inet/ppphosts.samp
		$(INS) -f $(INET) -m 0755 -u $(OWN) -g $(GRP) inet/pppfilter.samp
		$(INS) -f $(ETC) -m 0755 -u $(OWN) -g $(GRP) addrpool.samp
		$(INS) -f $(INET) -m 0755 -u $(OWN) -g $(GRP) inet/pppauth.samp
		$(INS) -f $(INET) -m 0755 -u $(OWN) -g $(GRP) inet/config
		$(INS) -f $(INET) -m 0644 -u $(OWN) -g $(GRP) inet/inet.dfl
		$(INS) -f $(INET) -m 0644 -u $(OWN) -g $(GRP) inet/nb.conf
		$(INS) -f $(INET) -m 0644 -u $(OWN) -g $(GRP) inet/gateways
		$(INS) -f $(INETCONF) -m 0644 -u $(OWN) -g $(GRP) \
			confnet.d/inet/interface
		$(INS) -f $(INETCONF) -m 0444 -u $(OWN) -g $(GRP) \
			confnet.d/inet/config.boot.sh
		$(INS) -f $(INETCONF) -m 0544 -u $(OWN) -g $(GRP) \
			confnet.d/inet/configure
		@for i in $(MKDIRS);\
		do\
			cd $$i;\
			/bin/echo "\n===== $(MAKE) -f $$i.mk install";\
			$(MAKE) -f $$i.mk install $(MAKEARGS);\
			cd ..;\
		done;\
		wait

$(DIRS):
		[ -d $@ ] || mkdir -p $@

clean:

clobber:

lintit:
