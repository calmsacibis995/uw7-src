#ident  "@(#)messages.mk	1.11"

LDAPTOP=	..

include $(CMDRULES)
include $(LDAPTOP)/local.defs

INSDIR   = /usr/lib/locale/C/LC_MESSAGES
CATALOGS = messages.str

all:
	gencat ldap.cat messages.str
	mkmsgs -o ldapmisc.str ldapmisc

install: all
	@[ -d $(USRLIB)/locale/C/LC_MESSAGES ] || \
		mkdir -p $(USRLIB)/locale/C/LC_MESSAGES
	$(INS) -f $(USRLIB)/locale/C/LC_MESSAGES ldap.cat
	$(INS) -f $(USRLIB)/locale/C/LC_MESSAGES ldap.cat.m
	$(INS) -f $(USRLIB)/locale/C/LC_MESSAGES ldapmisc
	@echo "Installed LDAP message catalogs"

clean:	
	$(RM) -f ldap.cat ldap.cat.m

clobber:	clean

lintit:
