#ident	"@(#)lprof:lprof.mk	1.10.1.13"

include $(CMDRULES)

include lprofinc.mk

all:  basicblk cmds 

basicblk:
	cd bblk/$(CPU); $(MAKE) -f bblk.mk

cmds:
	if test "$(NATIVE)" = "yes"; then \
		cd cmd; $(MAKE) -f cmd.mk ; \
	fi

install: all
	$(MAKE) target -f lprof.mk LPTARGET=install

lintit:
	$(MAKE) target -f lprof.mk LPTARGET=lintit

clean:
	$(MAKE) target -f lprof.mk LPTARGET=clean

clobber: clean
	$(MAKE) target -f lprof.mk LPTARGET=clobber

target:
	cd bblk/$(CPU); \
	$(MAKE) -f bblk.mk $(LPTARGET); \
	cd ../..; 
	if test "$(NATIVE)" = "yes"; then \
	   cd cmd; \
	   $(MAKE) -f cmd.mk $(LPTARGET) ; \
	   cd ..; \
	   cd libprof/$(CPU); \
	   $(MAKE) -f libprof.mk $(LPTARGET) ; \
	   cd ../..; \
	fi
