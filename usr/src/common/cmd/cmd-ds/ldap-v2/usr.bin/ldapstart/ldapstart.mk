#ident  "@(#)ldapstart.mk	1.6"

LDAPTOP=	../..

include $(CMDRULES)
include $(LDAPTOP)/local.defs

all:	ldapstart

install: all
	@[ -d $(USRBIN) ] || mkdir -p $(USRBIN)
	$(INS) -f $(USRBIN) ldapstart

clean:	

clobber:	clean

