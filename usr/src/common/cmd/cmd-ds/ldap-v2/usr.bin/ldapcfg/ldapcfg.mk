#ident  "@(#)ldapcfg.mk	1.4"

LDAPTOP=	../..

include $(CMDRULES)
include $(LDAPTOP)/local.defs
INSDIR=	$(USRLIB)/ldap

all: ldapcfg

install: all
	@[ -d $(INSDIR) ] || mkdir -p $(INS)
	$(INS) -f $(INSDIR) ldapcfg

clean:	

clobber:	clean

