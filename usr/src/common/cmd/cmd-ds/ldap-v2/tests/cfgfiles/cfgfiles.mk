#ident	"@(#)cfgfiles.mk	1.2"

LDAPTOP=     ../..
CURRENT_DIR= ./tests/cfgfiles

include $(CMDRULES)
include $(LDAPTOP)/local.defs

INSDIR = 		$(ROOT)/$(MACH)/usr/local/ldap-tests/cfgfiles

FILES=slapd.1.conf slapd.acl.conf slapd.at.conf slapd.oc.conf database1.conf

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
