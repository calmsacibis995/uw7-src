#ident  "@(#)ldapdbinfo.mk	1.4"

LDAPTOP=	../..

include $(CMDRULES)
include $(LDAPTOP)/local.defs
INSDIR=	$(USRLIB)/ldap

all: ldapdbinfo

install: all
	@[ -d $(INSDIR) ] || mkdir -p $(INS)
	$(INS) -f $(INSDIR) ldapdbinfo

clean:	

clobber:	clean

