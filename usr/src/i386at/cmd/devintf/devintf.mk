#ident	"@(#)devintf.mk	1.4"
#ident "$Header$"

include $(CMDRULES)

SUBMAKES=devices groups mkdtab flpyconf

foo	: all

.DEFAULT	:	
		for submk in $(SUBMAKES) ; \
		do \
			cd $$submk ; \
			$(MAKE) -f $$submk.mk $(MAKEARGS) $@ ; \
			cd .. ; \
		done
