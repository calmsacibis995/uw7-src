#ident	"@(#)ppp.mk	1.4"

include $(CMDRULES)

LIBDIR =	$(USRLIB)/ppp
OWN=		bin
GRP=		bin

SUBMKS = include pppd libpppcmd pppsh libedit ppptalk ppputils

all clean clobber config headinstall lintit fnames:
	@for d in $(SUBMKS); do \
		(cd $$d; echo "=== $(MAKE) -f $$d.mk $@"; \
		$(MAKE) -f $$d.mk $@ $(MAKEARGS)); \
	 done

install:	localinstall
	@for d in $(SUBMKS); do \
		(cd $$d; echo "=== $(MAKE) -f $$d.mk $@"; \
		$(MAKE) -f $$d.mk $@ $(MAKEARGS)); \
	 done

localinstall:
	$(INS) -f $(LIBDIR) -m 0444 -u $(OWN) -g $(GRP) rcscript

