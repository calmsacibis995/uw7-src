#		copyright	"%c%"
#ident	"@(#)Makefile	1.7"

#
# Makefile for the hpnp
#
# Delta Date:	10/2/97 10:24:25
#

include $(CMDRULES)


TOP	=	..

include ../common.mk


DEBUG	=

LOCALINC= -I$(LPINC)
LDLIBS = -lgen


OBJS	=	$(SRCS:.c=.o)


BIGCMDS	= src

LOCALCMDS = 	\
		hpnpinstall \
		hpnpcfg

BIGSCRIPTS =	\
		examples \
		cfg \
		testfiles

NLSGENF =	NLS/english/hpnp.gen
NLSHEADER =	./hpnp_msg.h
NLSSRCHDR =	./src/hpnp_msg.h

HPNPCAT =	hpnp_msg.sh

CMDS	=	$(HPNPCAT) $(LOCALCMDS) $(BIGCMDS)

.MUTEX:	$(CMDS) install

all:		$(CMDS)

hpnp_msg.sh:	$(NLSGENF)
		mkcatdefs -s hpnp $(NLSGENF) > /dev/null
		mkcatdefs hpnp $(NLSGENF) > /dev/null
		rm -f $(NLSSRCHDR)
		mv $(NLSHEADER) $(NLSSRCHDR)
		docats -d NLS all

hpnpinstall:	hpnp.sh $(NLSGENF)
		rm -f hpnpinstall
		echo "#!/bin/ksh" > hpnpinstall.top
		cat hpnpinstall.top hpnp_msg.sh hpnp.sh > hpnpinstall
		rm -f hpnpinstall.top

hpnpcfg:	hpnpcfg.sh $(NLSGENF)
		rm -f hpnpcfg
		echo "#!/bin/ksh" > hpnpcfg.top
		cat hpnpcfg.top hpnp_msg.sh hpnpcfg.sh > hpnpcfg
		rm -f hpnpcfg.top

install:	localinstall
	for cmd in $(BIGSCRIPTS) $(BIGCMDS); \
	do \
		cd ./$$cmd; \
		$(MAKE) SYMLINK="$(SYMLINK)" $(MAKEARGS) install; \
		cd ..; \
	done
	$(DOCATS) -d NLS $@

localinstall:
	for cmd in $(LOCALCMDS); \
	do \
		$(INS) -m $(EMODES) -u $(OWNER) -g $(GROUP) -f $(USRLIBHP) $$cmd; \
	done
	$(INS) -m $(EMODES) -u $(OWNER) -g $(GROUP) -f $(USRLIBHP) hpnp.model;


clean:		localclean
	for cmd in $(BIGCMDS); \
	do \
		cd ./$$cmd; \
		$(MAKE) $(MAKEARGS) clean; \
		cd ..; \
	done

localclean:
	rm -f hpnp_msg.sh ./src/hpnp_msg.h

clobber:	clean localclobber
	for cmd in $(BIGCMDS); \
	do \
		cd ./$$cmd; \
		$(MAKE) $(MAKEARGS) clobber; \
		cd ..; \
	done
	rm -f hpnp_msg.sh ./src/hpnp_msg.h

localclobber:
	for cmd in $(LOCALCMDS);	\
	do				\
		rm -f $$cmd;		\
	done

strip:
	for cmd in $(BIGCMDS); \
	do \
		cd ./$$cmd; \
		$(MAKE) $(MAKEARGS) strip; \
		cd ..; \
	done


#####
#
# Need the "nulldependency" because the targets in $(BIGCMDS)
# are directories. Without "nulldependency" make would always
# say the target is up to date.
#####

$(BIGCMDS):	nulldependency
	cd ./$@; $(MAKE) $(MAKEARGS) DEBUG="$(DEBUG)"

nulldependency:
	/bin/echo "Building hpnp source"


lintlib:
