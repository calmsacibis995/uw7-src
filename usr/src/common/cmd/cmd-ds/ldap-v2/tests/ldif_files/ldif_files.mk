#ident	"@(#)ldif_files.mk	1.2"

LDAPTOP=     ../..
CURRENT_DIR= ./tests/ldif_files

include $(CMDRULES)
include $(LDAPTOP)/local.defs

INSDIR = 		$(ROOT)/$(MACH)/usr/local/ldap-tests/ldif_files

FILES=SCO-EUROPE.ldif SCO-FRANCE.ldif SCO-GERMANY.ldif SCO-UK.ldif SCO-FRANCE.dodgy.ldif

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
