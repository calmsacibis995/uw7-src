# @(#)tools.mk	1.5

LDAPTOP=	../..

include $(CMDRULES)
include $(LDAPTOP)/local.defs

INSDIR=		$(USRBIN)

HDIR=		$(LDAPTOP)/include
LOCALINC=	-I$(HDIR) -I.

LDIR=		$(LDAPTOP)/usr.lib
LDLIBS	= -lldap -llber -lldif -lsocket -lnsl -lgen -lresolv -llog

STUFF	= ldapsearch ldapmodify ldapdelete ldapmodrdn 
LINKS	= ldapadd

all:	$(STUFF) $(LINKS)

$(STUFF):	$$@.o
	$(CC) -o $@ $(LDFLAGS) $@.o -L$(LDIR) $(LDLIBS) $(SHLIBS)

ldapadd:	ldapmodify
	$(RM) -f $@
	$(LN) -f ldapmodify ldapadd

install: all
	@[ -d $(INSDIR) ] || mkdir -p $(INSDIR)
	@for i in $(STUFF) ;\
		do\
			$(INS) -f $(INSDIR) $$i;\
		done
	$(LN) -f $(INSDIR)/ldapmodify $(INSDIR)/ldapadd

clean:	
	$(RM) -f *.o  ldapcmds

clobber:	clean
	$(RM) -f $(STUFF) $(LINKS) 

lintit:
	$(LINT) $(LINTFLAGS) $(CFLAGS) $(INCLIST) $(DEFLIST) *.c

FRC:

#
# Header dependencies
#
