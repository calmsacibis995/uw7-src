#ident	"@(#)etc.mk	1.7"

LDAPTOP=	..
CURRENT_DIR= ./etc

include $(CMDRULES)
include $(LDAPTOP)/local.defs

ETC_LDAP=       $(ETC)/ldap
DIRS = 		$(ETC_LDAP) 

TRAD_FILES=	slapd.conf slapd.at.conf slapd.oc.conf 
NEW_FILES=	ldap_defaults ldaptab slapd.acl.conf

all:		

install:	$(DIRS) $(TRAD_FILES) $(NEW_FILES)
		@for i in $(TRAD_FILES);\
		do \
			echo "making" $@ "in $(CURRENT_DIR)/$$i..."; \
			$(INS) -f $(ETC_LDAP) $$i; \
		done
		@for i in $(NEW_FILES);\
		do \
			echo "making" $@ "in $(CURRENT_DIR)/$$i..."; \
			$(INS) -f $(ETC_LDAP) $$i; \
		done


$(DIRS):
		[ -d $@ ] || mkdir -p $@

clean:

clobber:

lintit:
