#ident	"@(#)drf:drf.mk	1.1"
#

include $(CMDRULES)

DIRS =	cmd instcmd hcomp prt_files 

all .DEFAULT:
	@set -e;\
	for d in $(DIRS);\
	do\
		cd $$d;\
		$(MAKE) -f $$d.mk $@;\
		cd .. ;\
	done;

install: all
	@set -e;\
	for d in $(DIRS);\
	do\
		cd $$d;\
		$(MAKE) -f $$d.mk $@;\
		cd ..;\
	done;
