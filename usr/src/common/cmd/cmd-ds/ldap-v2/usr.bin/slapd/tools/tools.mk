#ident	"@(#)tools.mk	1.7"

LDAPTOP=	../../..

include $(CMDRULES)
include $(LDAPTOP)/local.defs

INSDIR=		$(USRBIN)

HDIR=		$(LDAPTOP)/include
DBHDIR=		$(LDAPTOP)/usr.lib/libdb/include
LOCALINC=	-I$(HDIR) -I$(DBHDIR)

LDIR=		$(LDAPTOP)/usr.lib
LDLIBS=		-lsocket -lnsl -lgen -lresolv -lldif -llber -lldap -lavl -lldbm -llthread $(LDBMLIB) -lthread 

SIMPLETOOLS=	ldbmcat ldif 

#
#	Some tools are linked to objects in the parent directory,
#	not very aesthetic, but there you go.  These objects are:
#
OBJS2   = 	../config.o ../ch_malloc.o ../backend.o ../charray.o \
		../aclparse.o ../schema.o ../result.o ../filterentry.o \
		../acl.o ../phonetic.o ../attr.o ../value.o ../entry.o \
		../dn.o ../filter.o ../str2filter.o ../ava.o ../init.o \
		../schemaparse.o ../regex.o 

OBJS2TOOLS = ldbmtest ldif2index ldif2id2entry ldif2id2children ldif2ldbm

all:		$(SIMPLETOOLS) $(OBJS2TOOLS) 

.c.o:
	$(CC) $(CFLAGS) $(INCLIST) $(DEFLIST) -c $<


$(SIMPLETOOLS):	$$@.o
		$(CC) -o $@ $(LDFLAGS) $@.o -L$(LDIR) $(LDLIBS) $(SHLIBS)


$(OBJS2TOOLS):	$$@.o ../libbackends.a $(OBJS2)
		$(CC) -O -o $@ $(LDFLAGS) $@.o $(OBJS2) ../libbackends.a \
			-L$(LDIR) $(LDLIBS) $(SHLIBS)

install:	all
		[ -d $(INSDIR) ] || mkdir -p $(INSDIR)
		@for i in $(SIMPLETOOLS) $(OBJS2TOOLS);\
		do\
			$(INS) -f $(INSDIR) $$i;\
		done

clean:
		$(RM) -f *.o 

clobber:	clean
		$(RM) -f $(SIMPLETOOLS) $(OBJS2TOOLS) 

lintit:
		$(LINT) $(LINTFLAGS) $(CFLAGS) $(INCLIST) $(DEFLIST) *.c

FRC:

#
# Header dependencies

ldbmcat.o:	ldbmcat.c \
		$(LDAPTOP)/include/ldbm.h \
		../slap.h \
		$(DBHDIR)/db.h

ldbmtest.o:	ldbmtest.c \
		$(LDAPTOP)/include/portable.h \
		$(LDAPTOP)/include/ldapconfig.h \
		../slap.h \
		../back-ldbm/back-ldbm.h

ldif.o:		ldif.c \
		$(INC)/lber.h \
		$(INC)/ldap.h \
		$(LDAPTOP)/include/ldif.h

ldif2id2children.o:	ldif2id2children.c \
		../slap.h \
		../back-ldbm/back-ldbm.h

ldif2id2entry.o:	ldif2id2entry.c \
		../slap.h \
		../back-ldbm/back-ldbm.h

ldif2index.o:	ldif2index.c \
		../slap.h 

ldif2ldbm.o:	ldif2ldbm.c \
		../slap.h \
		../back-ldbm/back-ldbm.h

sizecount.o:	sizecount.c \
		$(INC)/lber.h \
		$(INC)/ldap.h \
		$(LDAPTOP)/include/ldbm.h \
		$(LDAPTOP)/include/portable.h \
		$(DBHDIR)/db.h

