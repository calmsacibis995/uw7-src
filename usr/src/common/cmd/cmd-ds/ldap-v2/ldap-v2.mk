#ident "@(#)ldap-v2.mk	1.5"

LDAPTOP=	.
CURRENT_DIR=    .

include $(CMDRULES)
include $(LDAPTOP)/local.defs

DIRS=		etc usr.lib usr.bin messages rc-script gui-default-files

all install clean clobber lintit:
	@echo "Starting make" $@ "of LDAP";
	@for i in $(DIRS); \
	do \
		cd $$i;\
		echo "making" $@ "in $(CURRENT_DIR)/$$i..."; \
		$(MAKE) -f $$i.mk $@ $(MAKEARGS); cd .. ; \
	done;
	@echo "make" $@ "of LDAP is complete";
