#		copyright	"%c%"
#ident	"@(#)Makefile	1.11"

#
# Makefile for the Remote Printing (rlp) 
#
# Delta Date:	10/2/97 10:24:27
#

include $(CMDRULES)


TOP	=	..

include ../common.mk


DEBUG	=

MODELDIR= $(USRLIBLP)/model

LOCALINC= -I$(LPINC)
LDLIBS = -lgen


OBJS	=	$(SRCS:.c=.o)


BIGCMDS	= src

LOCALCMDS = 	\
		rlpconf

LOCALMDLS = 	\
		network

NLSGENF =	NLS/english/rlp.gen
NLSHEADER =	./rlp_msg.h
NLSSRCHDR =	./src/rlp_msg.h

RLPCAT =	rlp_msg.sh

BIGSCRIPTS =	

CMDS	=	$(RLPCAT) $(LOCALCMDS) $(LOCALMDLS) $(BIGCMDS)

.MUTEX:	$(CMDS) install

all:		$(CMDS)

rlp_msg.sh:	$(NLSGENF)
		mkcatdefs -s rlp $(NLSGENF) > /dev/null
		mkcatdefs rlp $(NLSGENF) > /dev/null
		rm -f $(NLSSRCHDR)
		mv $(NLSHEADER) $(NLSSRCHDR)
		docats -d NLS all

rlpconf:	rlpconf.sh $(NLSGENF)
		rm -f rlpconf
		echo "#!/bin/sh" > rlpconf.top
		cat rlpconf.top rlp_msg.sh rlpconf.sh > rlpconf
		rm -f rlpconf.top

network:	network.model
		cp network.model network

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
		$(INS) -m $(EMODES) -u $(OWNER) -g $(GROUP) -f $(USRLIB) $$cmd; \
	done
	for cmd in $(LOCALMDLS); \
	do \
		$(INS) -m $(EMODES) -u $(OWNER) -g $(GROUP) -f $(MODELDIR) $$cmd; \
	done


clean:		localclean
	for cmd in $(BIGCMDS); \
	do \
		cd ./$$cmd; \
		$(MAKE) $(MAKEARGS) clean; \
		cd ..; \
	done

localclean:
	for cmd in $(LOCALCMDS);	\
	do				\
		rm -f $$cmd;		\
	done
	rm -f rlp_msg.sh ./src/rlp_msg.h

clobber:	clean localclobber
	for cmd in $(BIGCMDS); \
	do \
		cd ./$$cmd; \
		$(MAKE) $(MAKEARGS) clobber; \
		cd ..; \
	done

localclobber:
	for cmd in $(LOCALCMDS);	\
	do				\
		rm -f $$cmd;		\
	done
	rm -f rlp_msg.sh ./src/rlp_msg.h

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
	/bin/echo "Building RLP source"


lintlib:
