#ident "@(#)tests.mk	1.2"

LDAPTOP=	..
CURRENT_DIR=    $LDAPTOP/tests

include $(CMDRULES)
include $(LDAPTOP)/local.defs

DIRS=cfgfiles ldif_files scripts

all install clean clobber:
	@echo "Starting make" $@ "of LDAP";
	@for i in $(DIRS); \
	do \
		cd $$i;\
		echo "making" $@ "in $(CURRENT_DIR)/$$i..."; \
		$(MAKE) -f $$i.mk $@ $(MAKEARGS); cd .. ; \
	done;

package:
	cd package;\
	$(MAKE) -f package.mk $@ $(MAKEARGS); cd ..;

image:
	cd package;\
	$(MAKE) -f package.mk $@ $(MAKEARGS); cd ..;

