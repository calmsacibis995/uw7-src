#	copyright	"%c%"

#
# Copyright  (c) 1985 AT&T
#	All Rights Reserved
#
#ident	"@(#)fmli:fmli.ci4.mk	1.2.2.4"
#ident "$Header$"
#

include $(CMDRULES)

CURSES_H=$(INC)

DIRS =	form menu oeu oh proc qued sys vt wish gen xx 

.DEFAULT:
	@set -e;\
	for d in $(DIRS);\
	do\
		cd $$d;\
		/bin/echo Making $@ in $$d subsystem;\
		$(MAKE) -f $$d.mk $(MAKEARGS) CURSES_H="$(CURSES_H)" GLOBALDEF="$(GLOBALDEF) -DPRE_CI5_COMPILE -DNO_MOUSE" EXTRA_LIBS="-lPW" $@;\
		cd ..;\
	done;\
	/bin/echo 'fmli.mk: finished making target "$@"'

