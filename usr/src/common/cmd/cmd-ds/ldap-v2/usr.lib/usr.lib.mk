# @(#)usr.lib.mk	1.5

CURRENT_DIR= ./usr.lib
LDAPTOP=        ..

include $(LIBRULES)
include $(LDAPTOP)/local.defs

DIRS=	liblog liblber libdb libldbm liblthread libavl libldap libldif \
	libldaptab

all install clean lintit clobber:
	@for i in $(DIRS); \
	do \
		cd $$i; \
		echo "making" $@ "in $(CURRENT_DIR)/$$i..."; \
		$(MAKE) -$(MAKEFLAGS) -f $$i.mk $@ $(MAKEARGS); cd .. ; \
	done;
