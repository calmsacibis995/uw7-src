#ident	"@(#)osrlibs:osrlibs.mk	1.2"
include $(CMDRULES)

OSRROOT = $(ROOT)/$(MACH)/OpenServer
OSR_USRLIB = $(OSRROOT)/usr/lib
OSR_LIB = $(OSRROOT)/lib

OWN=root
GRP=root

CCFLAGS = -O $(CC.PIC)

all:

install:
	if [ ! -d $(OSR_USRLIB) ] ; \
	then \
		mkdir -p $(OSR_USRLIB) ; \
	fi

	if [ ! -d $(OSR_LIB) ] ; \
	then \
		mkdir -p $(OSR_LIB) ; \
	fi

	@cd usr/lib; for d in `ls`; do \
		$(INS) -m 0755 -u $(OWN) -g $(GRP) -f $(OSR_USRLIB) $$d; \
	done

	@cd lib; for d in `ls`; do \
		$(INS) -m 0755 -u $(OWN) -g $(GRP) -f $(OSR_LIB) $$d; \
	done

	if [ ! -d $(USRLIB) ] ; \
	then \
		mkdir -p $(USRLIB) ; \
	fi

	@cd shlib; for d in `ls`; do \
		$(INS) -m 0755 -u $(OWN) -g $(GRP) -f $(USRLIB) $$d; \
	done


