# @(#)cmd-ds.mk	1.5

include $(CMDRULES)

SUBDIRS = ldap-v2

all install clean clobber lintit: 
	@for i in $(SUBDIRS) ; \
	do \
	cd $$i ; \
	$(MAKE) -f $$i.mk $(MAKEARGS) $@; cd .. ; \
	done

