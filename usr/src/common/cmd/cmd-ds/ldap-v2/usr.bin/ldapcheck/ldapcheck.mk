#ident  "@(#)ldapcheck.mk	1.6"

LDAPTOP=	../..

include $(CMDRULES)
include $(LDAPTOP)/local.defs
INSDIR=	$(USRBIN)

all: ldapcheck

install: all
	@[ -d $(INSDIR) ] || mkdir -p $(INSDIR)
	$(INS) -f $(INSDIR) ldapcheck

clean:	

clobber:	clean

