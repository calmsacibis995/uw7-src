#	copyright	"%c%"

#
#ident	"@(#)lp.mk	1.6"
#ident  "$Header$"
#
# Delta Date:	3/18/97 09:30:55
#
# Top level makefile for the LP Print Service component
#

include $(CMDRULES)

TOP	=	.

include ./common.mk

CHLVL	=	: chlvl

CMDDIR	=	./cmd
LIBDIR	=	./lib
INCDIR	=	./include
ETCDIR	=	./etc
MODELDIR=	./model
FILTDIR	=	./filter
CRONTABDIR=	./crontab
TERMINFODIR=	./terminfo
HPNPDIR=	./hpnp
RLPDIR=		./rlp

PLACES	=	$(LIBDIR) \
		$(CMDDIR) \
		$(ETCDIR) \
		$(MODELDIR) \
		$(FILTDIR) \
		$(CRONTABDIR) \
		$(TERMINFODIR) \
		$(HPNPDIR) \
		$(RLPDIR)

DIRS	= \
		$(VAR)/lp \
		$(VAR)/lp/logs \
		$(USRLIBLP) \
		$(USRLIBLP)/bin \
		$(USRLIBLP)/model \
		$(ETC)/lp \
		$(ETC)/lp/classes \
		$(ETC)/lp/forms \
		$(ETC)/lp/interfaces \
		$(ETC)/lp/printers \
		$(ETC)/lp/pwheels \
		$(VARSPOOL)/lp \
		$(VARSPOOL)/lp/admins \
		$(VARSPOOL)/lp/requests \
		$(VARSPOOL)/lp/system \
		$(VARSPOOL)/lp/fifos \
		$(VARSPOOL)/lp/fifos/private \
		$(VARSPOOL)/lp/fifos/public \
		$(USRLIBHP)		\
		$(USRLIBHP)/cfg		\
		$(USRLIBHP)/examples	\
		$(USRLIBHP)/testfiles

PRIMODE	=	0771
PUBMODE	=	0773

DEBUG	=

#.MUTEX: all strip
#.MUTEX: all
.MUTEX: all install

all:		libs
	$(MAKE) -f lp.mk $(MAKEARGS) DEBUG="$(DEBUG)" cmds etcs models filters \
		crontabs terminfos hpnps rlps

#####
#
# Doing "make install" from the top level will install stripped
# copies of the binaries. Doing "make install" from a lower level
# will install unstripped copies.
#####

# Removed call to strip as there appears to be a race condition in the top-level
# build which can cause strip to be called before the binary is built.

#install:	all strip 
install:	all
	#$(MAKE) -f lp.mk $(MAKEARGS) strip
	$(MAKE) -f lp.mk $(MAKEARGS) realinstall

realinstall:	dirs
	for d in $(PLACES); \
	do \
		cd $$d; \
		$(MAKE) $(MAKEARGS) install; \
		cd ..; \
	done

#####
#
# Lower level makefiles have "clobber" depend on "clean", but
# doing so here would be redundant.
#####
clean clobber:
	for d in $(PLACES); \
	do \
		cd $$d; \
		$(MAKE) $(MAKEARGS) $@; \
		cd ..; \
	done

strip:
	if [ -n "$(STRIP)" ]; \
	then \
		$(MAKE) STRIP=$(STRIP) -f lp.mk $(MAKEARGS) realstrip; \
	else \
		$(MAKE) STRIP=strip -f lp.mk $(MAKEARGS) realstrip; \
	fi

realstrip:
	for d in $(PLACES); \
	do \
		cd $$d; \
		$(MAKE) STRIP=$(STRIP) $(MAKEARGS) strip; \
		cd ..; \
	done

dirs:
	for d in $(DIRS); do if [ ! -d $$d ]; then mkdir $$d; fi; done
	$(CH)chown $(OWNER) $(DIRS)
	$(CH)chgrp $(GROUP) $(DIRS)
	$(CH)chmod $(DMODES) $(DIRS)
	-$(CHLVL) $(LEVEL) $(DIRS)
	$(CH)chmod $(PRIMODE) $(VARSPOOL)/lp/fifos/private
	$(CH)chmod $(PUBMODE) $(VARSPOOL)/lp/fifos/public
	-$(CHLVL) $(LEVEL) $(VARSPOOL)/lp/fifos/private
	-$(CHLVL) $(LEVEL) $(VARSPOOL)/lp/fifos/public
	-$(SYMLINK) /etc/lp $(VARSPOOL)/lp/admins/lp
	-$(SYMLINK) /usr/lib/lp/bin $(VARSPOOL)/lp/bin
	-$(SYMLINK) /var/lp/logs $(VARSPOOL)/lp/logs
	-$(SYMLINK) /var/lp/logs $(ETC)/lp/logs
	-$(SYMLINK) /usr/lib/lp/model $(VARSPOOL)/lp/model
	-$(CHLVL) $(LEVEL) $(VARSPOOL)/lp/admins/lp
	-$(CHLVL) $(LEVEL) $(VARSPOOL)/lp/bin
	-$(CHLVL) $(LEVEL) $(VARSPOOL)/lp/logs
	-$(CHLVL) $(LEVEL) $(ETC)/lp/logs
	-$(CHLVL) $(LEVEL) $(VARSPOOL)/lp/model

libs:
	cd $(LIBDIR); $(MAKE) $(MAKEARGS) DEBUG="$(DEBUG)" FUNCDCL="$(FUNCDCL)"

cmds:
	cd $(CMDDIR); $(MAKE) $(MAKEARGS) DEBUG="$(DEBUG)"

etcs:
	cd $(ETCDIR); $(MAKE) $(MAKEARGS) DEBUG="$(DEBUG)"

models:
	cd $(MODELDIR); $(MAKE) $(MAKEARGS) DEBUG="$(DEBUG)"

filters:
	cd $(FILTDIR); $(MAKE) $(MAKEARGS) DEBUG="$(DEBUG)"

crontabs:
	cd $(CRONTABDIR); $(MAKE) $(MAKEARGS) DEBUG="$(DEBUG)"

terminfos:
	cd $(TERMINFODIR); $(MAKE) $(MAKEARGS) DEBUG="$(DEBUG)"

hpnps:
	cd $(HPNPDIR); $(MAKE) $(MAKEARGS) DEBUG="$(DEBUG)"

rlps:
	cd $(RLPDIR); $(MAKE) $(MAKEARGS) DEBUG="$(DEBUG)"

lintit:   lintlib
	for d in $(PLACES); \
	do \
		cd $$d; \
		$(MAKE) $(MAKEARGS) DEBUG="$(DEBUG)" FUNCDCL="$(FUNCDCL)" LINT="$(LINT)" LINTFLAGS="$(LINTFLAGS)" lintit; \
		cd ..; \
	done

lintsrc:
	cd $(LIBDIR); $(MAKE) $(MAKEARGS) DEBUG="$(DEBUG)" FUNCDCL="$(FUNCDCL)" LINT="$(LINT)" LINTFLAGS="$(LINTFLAGS)" lintsrc

lintlib:
	for d in $(PLACES); \
	do \
		cd $$d; \
		$(MAKE) $(MAKEARGS) DEBUG="$(DEBUG)" FUNCDCL="$(FUNCDCL)" LINT="$(LINT)" LINTFLAGS="$(LINTFLAGS)" lintlib; \
		cd ..; \
	done
