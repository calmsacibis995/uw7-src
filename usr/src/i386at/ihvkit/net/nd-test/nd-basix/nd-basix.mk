# inc dir is a kludge for ANT - integrate more fully into etet
include $(CMDRULES)

all: pkg

build clean :
	cd src/gui; $(MAKE) -f gui.mk $@
	cd src/dlpiut; $(MAKE) -f dlpiut.mk
	cd src/ndsu; $(MAKE) -f ndsu.mk

pkg: 
	cd src/dlpiut; $(MAKE) -f dlpiut.mk
	cd src/guic; $(MAKE) -f guic.mk
	cd src/ndsu; $(MAKE) -f ndsu.mk
	cp -f src/dlpiut/dlpiut bin/
	cp -f src/dlpiut/inc/libant.sh inc/
	cp -f src/dlpiut/inc/str.sh inc/
	cp -f src/ndsu/ndsu bin/
	cp -f src/gui/* bin/
	cp -f src/guic/start_menu bin/
	cp -f src/guic/unc_read bin/
	cp -f src/dlpiut/inc/xpg3sh/mytet.h inc/xpg3sh
	cp -f ${TET_ROOT}/bin/tcc bin/
	cp -f ${TET_ROOT}/lib/xpg3sh/tcm.sh lib/xpg3sh
	cp -f ${TET_ROOT}/lib/xpg3sh/tetapi.sh lib/xpg3sh


CLOBBER: clean clean_pkg
