#ident	"@(#)usr.bin.mk	1.5"

CURRENT_DIR=    ./usr.bin
LDAPTOP=	..

include $(CMDRULES)
include $(LDAPTOP)/local.defs

DIRS	=	slapd slurpd tools tests ldapstart ldapstop ldapcheck \
		ldapinfo ldaptabProcedures ldapcfg ldapdbinfo getreplicainfo

all install clean clobber lintit:
	@for i in $(DIRS); \
	do \
		cd $$i; \
		echo "making" $@ "in $(CURRENT_DIR)/$$i..."; \
		$(MAKE) -$(MAKEFLAGS) -f $$i.mk $@ $(MAKEARGS); cd .. ; \
	done;
