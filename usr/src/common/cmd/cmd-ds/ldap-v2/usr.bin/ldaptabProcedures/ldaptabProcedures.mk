#ident  "@(#)ldaptabProcedures.mk	1.5"

LDAPTOP=	../..

include $(CMDRULES)
include $(LDAPTOP)/local.defs
INSDIR=	$(USRLIB)/ldap

all: ldaptabProcedures

install: all
	@[ -d $(INSDIR) ] || mkdir -p $(INS)
	$(INS) -f $(INSDIR) ldaptabProcedures

clean:	

clobber:	clean

