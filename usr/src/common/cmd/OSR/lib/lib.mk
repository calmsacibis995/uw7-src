#ident	"@(#)OSRcmds:lib/lib.mk	1.1"
#

include $(CMDRULES)

DIRS =	libos misc libgen

all .DEFAULT:
	@set -e;\
	for d in $(DIRS);\
	do\
		cd $$d;\
		echo "Executing makefile in sub directory $$d ... ";\
		$(MAKE) -f *.mk $@ install;\
		cd .. ;\
	done;

install: all

clobber:
	@set -e;\
	for d in $(DIRS);\
	do\
		cd $$d;\
		$(MAKE) -f *.mk clobber;\
		cd ..;\
	done;
