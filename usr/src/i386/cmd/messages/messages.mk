#	copyright	"%c%"

#ident	"@(#)messages.mk	1.2"

include $(CMDRULES)

foo : all

.DEFAULT : 
	for submk in `ls` ; \
	do \
		[ -f $$submk/msgs.mk ] || continue ; \
	 	cd $$submk ; \
	 	$(MAKE) -f msgs.mk $@ ; \
	 	cd .. ; \
	done

