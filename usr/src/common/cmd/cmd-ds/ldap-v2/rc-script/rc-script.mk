#ident  "@(#)rc-script.mk	1.2"

LDAPTOP=	..

include $(CMDRULES)

OWN=		root
GRP=		sys

SCRIPT = ldap
INSDIR = $(ETC)/init.d

all:	

install: all
	@[ -d $(INSDIR) ] || mkdir -p $(INSDIR)
	$(INS) -f $(INSDIR) $(SCRIPT)
	@echo "Installed LDAP startup/shutdown scripts"

clean:	

clobber:	clean


