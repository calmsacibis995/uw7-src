#ident	"@(#)scripts.mk	1.2"

LDAPTOP=     ../..
CURRENT_DIR= ./tests/scripts

include $(CMDRULES)
include $(LDAPTOP)/local.defs

INSDIR = 		$(ROOT)/$(MACH)/usr/local/ldap-tests/scripts

FILES=check-directory ldap-test-globals check-rt-dir

all:		

install:	$(INSDIR) $(FILES)
		@for i in $(FILES);\
		do \
			echo "making" $@ "in $(CURRENT_DIR)/$$i..."; \
			$(INS) -f $(INSDIR) $$i; \
		done


$(INSDIR):
		[ -d $@ ] || mkdir -p $@

clean:

clobber:

lintit:
