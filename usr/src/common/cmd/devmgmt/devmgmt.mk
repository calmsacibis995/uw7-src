#	copyright	"%c%"

#ident	"@(#)devmgmt:common/cmd/devmgmt/devmgmt.mk	1.7.6.2"
#ident "$Header$"

include $(CMDRULES)

SUBMAKES=devattr getdev getdgrp listdgrp devreserv devfree data putdev putdgrp getvol ddbconv

foo		: all

.DEFAULT	:	
		for submk in $(SUBMAKES) ; \
		do \
		    cd $$submk ; \
		    $(MAKE) -f $$submk.mk $(MAKEARGS) $@ ; \
		    cd .. ; \
		done
