# @(#)tests.mk	1.4

CURRENT_DIR= ./usr.bin/tests
LDAPTOP=	../..

include $(CMDRULES)
include $(LDAPTOP)/local.defs

DIRS=		libavl liblber libldap 

all install clobber clean lintit:
	@for i in $(DIRS); \
	do \
		cd $$i;\
		echo "making" $@ "in $(CURRENT_DIR)/$$i..."; \
		$(MAKE) -f $$i.mk $@ $(MAKEARGS); cd .. ; \
	done;
