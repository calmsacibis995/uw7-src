#ident  @(#)getreplicainfo.mk	1.3

LDAPTOP=	../..

include $(CMDRULES)
include $(LDAPTOP)/local.defs

INSDIR=		$(USRLIB)/ldap

HDIR=		$(LDAPTOP)/include
LOCALINC=	-I$(HDIR) -I.

LDIR=		$(LDAPTOP)/usr.lib
LDLIBS	= -lldap -llber -lldif -lsocket -lnsl -lgen -lresolv

MYTOOLS = getreplicainfo

all:	$(MYTOOLS)

$(MYTOOLS):	$$@.o
	$(CC) -o $@ $@.o

install: all
	@[ -d $(INSDIR) ] || mkdir -p $(INSDIR)
	@for i in $(MYTOOLS) ;\
		do\
			$(INS) -f $(INSDIR) $$i;\
		done

clean:	
	$(RM) -f *.o

clobber:	clean
	$(RM) -f $(MYTOOLS)

lintit:
	$(LINT) $(LINTFLAGS) $(CFLAGS) $(INCLIST) $(DEFLIST) *.c

FRC:

