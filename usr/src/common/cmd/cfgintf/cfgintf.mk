#ident	"@(#)cfgintf:common/cmd/cfgintf/cfgintf.mk	1.1.5.3"
#ident "$Header$"

include $(CMDRULES)

# SUBMAKES=system summary logins
SUBMAKES=system summary di

foo		: all

.DEFAULT	:	
		for submk in $(SUBMAKES) ; \
		do \
		    if [ -d $$submk ] ; \
		    then \
		    	cd $$submk ; \
		    	$(MAKE) -f $$submk.mk $@ $(MAKEARGS); \
		    	cd .. ; \
		    fi \
		done
