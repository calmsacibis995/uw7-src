#ident	"@(#)gui-default-files.mk	1.3"

LDAPTOP=	..
CURRENT_DIR= ./gui-default-files

include $(CMDRULES)
include $(LDAPTOP)/local.defs

INSDIR=       	$(USRLIB)/ldap/default-files
DIRS=		$(USRLIB)/ldap/default-files


FILES=		slapd.at.conf slapd.oc.conf slapd.acl.conf slapd.conf.entries

all:		

install:	$(DIRS)
		@for i in $(FILES);\
		do \
			echo "making" $@ "in $(CURRENT_DIR)/$$i..."; \
			$(INS) -f $(INSDIR) $$i; \
		done

$(DIRS):
	[ -d $@ ] || mkdir -p $@

clean:

clobber:

lintit:
