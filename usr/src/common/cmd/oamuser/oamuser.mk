#ident	"@(#)oamuser.mk	1.3"
#ident  "$Header$"

include $(CMDRULES)

DIRS = lib group user

all clean clobber lintit size strip:
	@for i in $(DIRS) ;\
	do \
		echo "\tcd $$i && $(MAKE) $(MAKEARGS) $@" ;\
		cd $$i ;\
		$(MAKE) $(MAKEARGS) $(@) ; \
		cd .. ; \
	done

install :  all
	-[ -d $(USRSADM) ] || mkdir -p $(USRSADM)
	-[ -d $(ETC)/skel ] || mkdir -p $(ETC)/skel
	@for i in $(DIRS) ;\
	do \
		echo "\tcd $$i && $(MAKE) $(MAKEARGS) $@" ;\
		cd $$i ;\
		$(MAKE) $(MAKEARGS) $(@) ; \
		cd .. ; \
	done
