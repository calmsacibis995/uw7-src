#ident  "@(#)ldapinfo.mk	1.6"

LDAPTOP=	../..

include $(CMDRULES)
include $(LDAPTOP)/local.defs

all:	ldapinfo

install: all
	@[ -d $(USRLIB) ] || mkdir -p $(USRLIB)
	$(INS) -f $(USRLIB)/ldap ldapinfo

clean:	

clobber:	clean

