# @(#)slurpd.mk	1.8

LDAPTOP=	../..

include $(CMDRULES)
include $(LDAPTOP)/local.defs

INSDIR=		$(USRLIB)/ldap

HDIR=		$(LDAPTOP)/include
LOCALINC=	-I$(HDIR) -I.

LDIR=		$(LDAPTOP)/usr.lib
LDLIBS=		-lldap -llber -lldif -llthread \
		-lsocket -lnsl -lgen $(LDBMLIB) -lthread -lresolv -llog

OBJECTS		= admin.o args.o ch_malloc.o config.o detach.o \
		fm.o globals.o ldap_op.o lock.o main.o re.o \
		reject.o replica.o replog.o ri.o rq.o sanity.o st.o \
		tsleep.o pidfile.o

all: 		slurpd

slurpd:		$(OBJECTS) 
		$(CC) -o slurpd  $(LDFLAGS) -L$(LDIR) $(OBJECTS) \
			$(LDLIBS) $(SHLIBS)

install:	all
		[ -d $(INSDIR) ] || mkdir -p $(INSDIR)
		$(INS) -f $(INSDIR) slurpd

clean:
		$(RM) -f $(OBJECTS) 

clobber:	clean
		$(RM) -f slurpd

lintit:
		$(LINT) $(LINTFLAGS) $(CFLAGS) $(INCLIST) $(DEFLIST) *.c

FRC:

#
# Header dependencies
#

admin.o: 	admin.c \
		$(INC)/lber.h \
		$(INC)/ldap.h \
		$(LDAPTOP)/include/lthread.h \
		$(LDAPTOP)/include/portable.h \
		$(LDAPTOP)/include/ldapconfig.h \
		$(LDAPTOP)/include/ldif.h \
		globals.h \
		slurp.h 

args.o: 	args.c \
		$(INC)/lber.h \
		$(INC)/ldap.h \
		$(LDAPTOP)/include/lthread.h \
		$(LDAPTOP)/include/portable.h \
		$(LDAPTOP)/include/ldapconfig.h \
	 	$(LDAPTOP)/include/ldif.h \
		globals.h \
		slurp.h 

ch_malloc.o: 	ch_malloc.c \
		$(INC)/lber.h \
		$(INC)/ldap.h \
		../slapd/slap.h \
		$(LDAPTOP)/include/avl.h \
		$(LDAPTOP)/include/lthread.h \
		$(LDAPTOP)/include/ldif.h 

config.o: 	config.c \
		$(INC)/lber.h \
		$(INC)/ldap.h \
		$(LDAPTOP)/include/lthread.h \
		$(LDAPTOP)/include/portable.h \
		$(LDAPTOP)/include/ldapconfig.h \
		$(LDAPTOP)/include/ldif.h \
		globals.h \
		slurp.h 

detach.o: 	detach.c \
		$(LDAPTOP)/include/portable.h 

fm.o: 		fm.c \
		slurp.h \
		$(INC)/lber.h \
		$(INC)/ldap.h \
		$(LDAPTOP)/include/lthread.h \
		$(LDAPTOP)/include/portable.h \
		$(LDAPTOP)/include/ldapconfig.h \
		$(LDAPTOP)/include/ldif.h \
		globals.h 

globals.o: 	globals.c \
		slurp.h \
		$(INC)/lber.h \
		$(INC)/ldap.h \
		$(LDAPTOP)/include/lthread.h \
		$(LDAPTOP)/include/portable.h \
		$(LDAPTOP)/include/ldapconfig.h \
		$(LDAPTOP)/include/ldif.h \
		globals.h 

ldap_op.o: 	ldap_op.c \
		$(INC)/lber.h \
		$(INC)/ldap.h  \
		$(LDAPTOP)/include/portable.h \
		$(LDAPTOP)/include/lthread.h \
		$(LDAPTOP)/include/ldapconfig.h \
		$(LDAPTOP)/include/ldif.h \
		slurp.h 

lock.o: 	lock.c \
		$(LDAPTOP)/include/portable.h \
		../slapd/slap.h \
		$(LDAPTOP)/include/avl.h \
		$(INC)/lber.h \
		$(INC)/ldap.h \
		$(LDAPTOP)/include/lthread.h \
		$(LDAPTOP)/include/ldif.h 

main.o: 	main.c \
		slurp.h \
		$(INC)/lber.h \
		$(INC)/ldap.h \
		$(LDAPTOP)/include/lthread.h \
		$(LDAPTOP)/include/portable.h \
		$(LDAPTOP)/include/ldapconfig.h \
		$(LDAPTOP)/include/ldif.h \
		globals.h \
		slurp.h 

re.o: 		re.c \
		../slapd/slap.h \
		$(LDAPTOP)/include/avl.h \
		$(INC)/lber.h \
		$(INC)/ldap.h \
		$(LDAPTOP)/include/lthread.h \
		$(LDAPTOP)/include/ldif.h \
		$(LDAPTOP)/include/portable.h \
		$(LDAPTOP)/include/ldapconfig.h \
		$(LDAPTOP)/include/ldif.h \
 		globals.h \
		slurp.h

reject.o: 	reject.c \
		slurp.h \
		$(INC)/lber.h \
		$(INC)/ldap.h \
		$(LDAPTOP)/include/lthread.h \
		$(LDAPTOP)/include/portable.h \
		$(LDAPTOP)/include/ldapconfig.h \
		$(LDAPTOP)/include/ldif.h \
		globals.h 

replica.o: 	replica.c \
		slurp.h \
		$(INC)/lber.h \
		$(INC)/ldap.h \
		$(LDAPTOP)/include/lthread.h \
		$(LDAPTOP)/include/portable.h \
 		$(LDAPTOP)/include/ldapconfig.h \
		$(LDAPTOP)/include/ldif.h \
		globals.h 

replog.o: 	replog.c \
		slurp.h \
		$(INC)/lber.h \
 		$(INC)/ldap.h \
		$(LDAPTOP)/include/lthread.h \
		$(LDAPTOP)/include/portable.h \
		$(LDAPTOP)/include/ldapconfig.h \
		$(LDAPTOP)/include/ldif.h \
		globals.h 

ri.o:		ri.c \
		slurp.h \
		$(INC)/lber.h \
		$(INC)/ldap.h \
		$(LDAPTOP)/include/lthread.h \
		$(LDAPTOP)/include/portable.h \
		$(LDAPTOP)/include/ldapconfig.h \
		$(LDAPTOP)/include/ldif.h \
		globals.h 

rq.o:		rq.c \
		slurp.h \
		$(INC)/lber.h \
		$(INC)/ldap.h \
		$(LDAPTOP)/include/lthread.h \
		$(LDAPTOP)/include/portable.h \
		$(LDAPTOP)/include/ldapconfig.h \
		$(LDAPTOP)/include/ldif.h \
		globals.h 

sanity.o: 	sanity.c \
		slurp.h \
		$(INC)/lber.h \
		$(INC)/ldap.h \
		$(LDAPTOP)/include/lthread.h \
		$(LDAPTOP)/include/portable.h \
		$(LDAPTOP)/include/ldapconfig.h \
		$(LDAPTOP)/include/ldif.h \
		globals.h 

st.o:		st.c \
		slurp.h \
		$(INC)/lber.h \
		$(INC)/ldap.h \
		$(LDAPTOP)/include/lthread.h \
		$(LDAPTOP)/include/portable.h \
		$(LDAPTOP)/include/ldapconfig.h \
		$(LDAPTOP)/include/ldif.h \
		globals.h 

tsleep.o: 	tsleep.c \
		slurp.h \
		$(INC)/lber.h \
		$(INC)/ldap.h \
		$(LDAPTOP)/include/lthread.h \
		$(LDAPTOP)/include/portable.h \
		$(LDAPTOP)/include/ldapconfig.h \
		$(LDAPTOP)/include/ldif.h \
		globals.h 

pidfile.o:	pidfile.c \
		slurp.h \
		globals.h
