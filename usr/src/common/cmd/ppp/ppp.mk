#ident	"@(#)ppp.mk	1.5"

include $(CMDRULES)

SUBMKS = md5 ppp lcp ip bap ccp stac pap chap pred1

all clean clobber config headinstall install lintit fnames:
	@for d in $(SUBMKS); do \
		if [ -d $$d ] ; then \
			(cd $$d; echo "=== $(MAKE) -f $$d.mk $@"; \
			$(MAKE) -f $$d.mk $@ $(MAKEARGS)); \
		fi \
	 done

