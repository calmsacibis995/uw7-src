#ident  "@(#)ldapstop.mk	1.6"

LDAPTOP=	../..

include $(CMDRULES)
include $(LDAPTOP)/local.defs

all:	ldapstop

install: all
	@[ -d $(USRBIN) ] || mkdir -p $(USRBIN)
	$(INS) -f $(USRBIN) ldapstop

clean:	

clobber:	clean

