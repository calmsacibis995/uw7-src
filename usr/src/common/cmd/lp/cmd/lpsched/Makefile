#ident	"@(#)Makefile	1.3"
#ident "$Header$"
#
# Makefile for LP scheduler trio
#

include $(CMDRULES)

TOP	=	../..

include ../../common.mk


DEBUG	=

##########
#
# If you add directories other than one level deep
# you'll have to change the code below.
##########

CMDDIRS= \
		./lpsched \
		./lpNet



all:		cmds

install clean clobber:
	for dir in $(CMDDIRS);do \
		if [ -d $$dir ]; then \
			cd $$dir; \
			$(MAKE) $(MAKEARGS) $@; \
			cd ..; \
		fi; \
	done

strip:
	for dir in $(CMDDIRS);do \
		if [ -d $$dir ]; then \
			cd $$dir; \
			$(MAKE) STRIP=$(STRIP) $(MAKEARGS) strip; \
			cd ..; \
		fi; \
	done

cmds:
	for dir in $(CMDDIRS);do \
		if [ -d $$dir ]; then \
			cd $$dir; \
			$(MAKE) $(MAKEARGS) DEBUG="$(DEBUG)"; \
			cd ..; \
		fi; \
	done

lintit:
	for dir in $(CMDDIRS); \
	do \
		cd $$dir; \
		$(MAKE) $(MAKEARGS) DEBUG="$(DEBUG)" LINT="$(LINT)" lintit; \
		cd ..; \
	done

lintlib:
	for dir in $(CMDDIRS); \
	do \
		cd $$dir; \
		$(MAKE) $(MAKEARGS) DEBUG="$(DEBUG)" LINT="$(LINT)" lintlib; \
		cd ..; \
	done
