#	copyright	"%c%"

# Copyright  (c) 1985 AT&T
#	All Rights Reserved
#
#ident	"@(#)fmli:fmli.mk	1.8.5.5"
#

include $(CMDRULES)

CURSES_H=$(INC)

DIRS =	form menu oeu oh proc qued sys vt wish xx msg

all .DEFAULT:
	@set -e;\
	for d in $(DIRS);\
	do\
		cd $$d;\
		/bin/echo "\nMaking $@ in $$d subsystem\n";\
		$(MAKE) -f $$d.mk CURSES_H="$(CURSES_H)" $(MAKEARGS) $@;\
		cd ..;\
	done;\
	/bin/echo 'fmli.mk: finished making target "$@"'

install: all
	@set -e;\
	for d in $(DIRS);\
	do\
		cd $$d;\
		/bin/echo "\Making $@ in $$d subsystem\n";\
		$(MAKE) -f $$d.mk CURSES_H="$(CURSES_H)" $(MAKEARGS) $@;\
		cd ..;\
	done;\
	/bin/echo 'fmli.mk: finished making target "$@"'
