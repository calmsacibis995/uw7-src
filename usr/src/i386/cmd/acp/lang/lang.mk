#ident	"@(#)acp:i386/cmd/acp/lang/lang.mk	1.1"
include $(CMDRULES)

OWN=bin
GRP=bin

CCFLAGS = -O $(CC.PIC)
LANGDIRS = lang nls

all:

install:
	for d in `find $(LANGDIRS) -print` ; do \
                if [ -d $$d ] ; \
                then \
                        if [ ! -d $(USRLIB)/$$d ] ; \
                        then \
				 mkdir -p $(USRLIB)/$$d ; \
                        fi ; \
		else  \
			pos=`pwd`; \
			cd `dirname $$d`; \
			 $(INS) -m 0644 -u $(OWN) -g $(GRP) -f $(USRLIB)/`dirname $$d` `basename $$d`; \
			cd $$pos ; \
		fi ; \
	done ; \
	if [ ! -d $(ETC)/default ] ; \
		then \
		mkdir -p $(ETC)/default ; \
	fi ; \
	cd default; \
	$(INS) -m 0644 -u $(OWN) -g $(GRP) -f $(ETC)/default lang; 


