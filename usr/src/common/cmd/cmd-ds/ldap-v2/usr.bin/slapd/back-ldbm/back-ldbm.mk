# @(#)back-ldbm.mk	1.5

LDAPTOP = ../../..

include $(LIBRULES)
include $(LDAPTOP)/local.defs

HDIR=		$(LDAPTOP)/include
SLAPDIR=	..
LOCALINC=	-I$(HDIR) -I$(SLAPDIR)

LIBRARY	= libback-ldbm.a
OBJECTS	= idl.o add.o search.o cache.o dbcache.o dn2id.o id2entry.o \
		index.o id2children.o nextid.o abandon.o compare.o \
		modify.o modrdn.o delete.o init.o config.o bind.o attr.o \
		filterindex.o unbind.o close.o

all:	$(LIBRARY)

install: all

$(LIBRARY):	$(OBJECTS)
	$(AR) $(ARFLAGS) $(LIBRARY) $(OBJECTS)

clean:
	$(RM) -f *.o 

clobber:	clean
	$(RM) -f $(LIBRARY)

lintit:
	$(LINT) $(LINTFLAGS) $(CFLAGS) $(INCLIST) $(DEFLIST) *.c

FRC:

#
# Header dependencies
#

idl.o: 		idl.c \
		../slap.h \
		$(INC)/lber.h \
		$(INC)/ldap.h \
		$(LDAPTOP)/include/avl.h \
		$(LDAPTOP)/include/lthread.h \
		$(LDAPTOP)/include/ldif.h \
		$(LDAPTOP)/include/ldapconfig.h \
		$(LDAPTOP)/include/ldbm.h \
		back-ldbm.h 

add.o: 		add.c \
		../slap.h \
		$(INC)/lber.h \
		$(INC)/ldap.h \
		$(LDAPTOP)/include/avl.h \
		$(LDAPTOP)/include/lthread.h \
		$(LDAPTOP)/include/ldif.h \
		$(LDAPTOP)/include/ldbm.h \
		back-ldbm.h 

search.o: 	search.c \
		../slap.h \
		$(INC)/lber.h \
		$(INC)/ldap.h \
		$(LDAPTOP)/include/avl.h \
		$(LDAPTOP)/include/lthread.h \
		$(LDAPTOP)/include/ldif.h \
		$(LDAPTOP)/include/ldbm.h \
		back-ldbm.h 

cache.o: 	cache.c \
		../slap.h \
		$(INC)/lber.h \
 		$(INC)/ldap.h \
		$(LDAPTOP)/include/avl.h \
		$(LDAPTOP)/include/lthread.h \
	 	$(LDAPTOP)/include/ldif.h \
		$(LDAPTOP)/include/ldbm.h \
		back-ldbm.h 

dbcache.o: 	dbcache.c \
		../slap.h \
		$(INC)/lber.h \
 		$(INC)/ldap.h \
	 	$(LDAPTOP)/include/avl.h \
		$(LDAPTOP)/include/lthread.h \
 		$(LDAPTOP)/include/ldif.h \
		$(LDAPTOP)/include/ldbm.h \
		$(LDAPTOP)/include/ldapconfig.h \
		$(LDAPTOP)/include/portable.h \
		back-ldbm.h 

dn2id.o: 	dn2id.c \
		../slap.h \
		$(INC)/lber.h \
 		$(INC)/ldap.h \
		$(LDAPTOP)/include/avl.h \
		$(LDAPTOP)/include/lthread.h \
	 	$(LDAPTOP)/include/ldif.h \
		$(LDAPTOP)/include/ldbm.h \
		back-ldbm.h 

id2entry.o: 	id2entry.c \
		../slap.h \
		$(INC)/lber.h \
 		$(INC)/ldap.h \
		$(LDAPTOP)/include/avl.h \
		$(LDAPTOP)/include/lthread.h \
 		$(LDAPTOP)/include/ldif.h \
		$(LDAPTOP)/include/ldbm.h \
		back-ldbm.h 

index.o: 	index.c \
		../slap.h \
		$(INC)/lber.h \
 		$(INC)/ldap.h \
		$(LDAPTOP)/include/avl.h \
		$(LDAPTOP)/include/lthread.h \
	 	$(LDAPTOP)/include/ldif.h \
		$(LDAPTOP)/include/ldbm.h \
		back-ldbm.h 

id2children.o: 	id2children.c \
		../slap.h \
 		$(INC)/lber.h \
		$(INC)/ldap.h \
		$(LDAPTOP)/include/avl.h \
	 	$(LDAPTOP)/include/lthread.h \
		$(LDAPTOP)/include/ldif.h \
		$(LDAPTOP)/include/ldbm.h \
		back-ldbm.h

nextid.o: 	nextid.c \
		../slap.h \
		$(INC)/lber.h \
		$(INC)/ldap.h \
		$(LDAPTOP)/include/avl.h \
		$(LDAPTOP)/include/lthread.h \
		$(LDAPTOP)/include/ldif.h \
		$(LDAPTOP)/include/ldbm.h \
		back-ldbm.h 

abandon.o: 	abandon.c 

compare.o: 	compare.c \
		../slap.h \
		$(INC)/lber.h \
		$(INC)/ldap.h \
		$(LDAPTOP)/include/avl.h \
		$(LDAPTOP)/include/lthread.h \
		$(LDAPTOP)/include/ldif.h \
		$(LDAPTOP)/include/ldbm.h \
		back-ldbm.h 

modify.o: 	modify.c \
		../slap.h \
		$(INC)/lber.h \
		$(INC)/ldap.h \
		$(LDAPTOP)/include/avl.h \
		$(LDAPTOP)/include/lthread.h \
		$(LDAPTOP)/include/ldif.h \
		$(LDAPTOP)/include/ldbm.h \
		back-ldbm.h 

modrdn.o: 	modrdn.c \
		../slap.h \
		$(INC)/lber.h \
		$(INC)/ldap.h \
		$(LDAPTOP)/include/avl.h \
		$(LDAPTOP)/include/lthread.h \
	 	$(LDAPTOP)/include/ldif.h \
		$(LDAPTOP)/include/ldbm.h \
		back-ldbm.h 

delete.o: 	delete.c \
		../slap.h \
		$(INC)/lber.h \
 		$(INC)/ldap.h \
		$(LDAPTOP)/include/avl.h \
		$(LDAPTOP)/include/lthread.h \
	 	$(LDAPTOP)/include/ldif.h \
		$(LDAPTOP)/include/ldbm.h \
		back-ldbm.h 

init.o: 	init.c \
		../slap.h \
		$(INC)/lber.h \
 		$(INC)/ldap.h \
		$(LDAPTOP)/include/avl.h \
		$(LDAPTOP)/include/lthread.h \
	 	$(LDAPTOP)/include/ldif.h \
		$(LDAPTOP)/include/ldbm.h \
		back-ldbm.h 

config.o: 	config.c \
		../slap.h \
		$(INC)/lber.h \
 		$(INC)/ldap.h \
		$(LDAPTOP)/include/avl.h \
		$(LDAPTOP)/include/lthread.h \
	 	$(LDAPTOP)/include/ldif.h \
		$(LDAPTOP)/include/ldbm.h \
		back-ldbm.h 

bind.o: 	bind.c \
		../slap.h \
		$(INC)/lber.h \
 		$(INC)/ldap.h \
		$(LDAPTOP)/include/avl.h \
		$(LDAPTOP)/include/lthread.h \
 		$(LDAPTOP)/include/ldif.h \
		$(LDAPTOP)/include/ldbm.h \
		back-ldbm.h 

attr.o: 	attr.c \
		../slap.h \
		$(INC)/lber.h \
 		$(INC)/ldap.h \
		$(LDAPTOP)/include/avl.h \
		$(LDAPTOP)/include/lthread.h \
	 	$(LDAPTOP)/include/ldif.h \
		$(LDAPTOP)/include/ldbm.h \
		back-ldbm.h 

filterindex.o: 	filterindex.c \
		../slap.h \
		$(INC)/lber.h \
		$(INC)/ldap.h \
		$(LDAPTOP)/include/avl.h \
	 	$(LDAPTOP)/include/lthread.h \
		$(LDAPTOP)/include/ldif.h \
	 	$(LDAPTOP)/include/ldbm.h \
		back-ldbm.h 

unbind.o: 	unbind.c \
		../slap.h \
		$(INC)/lber.h \
 		$(INC)/ldap.h \
		$(LDAPTOP)/include/avl.h \
		$(LDAPTOP)/include/lthread.h \
		$(LDAPTOP)/include/ldif.h 

close.o: 	close.c \
		../slap.h \
		$(INC)/lber.h \
 		$(INC)/ldap.h \
		$(LDAPTOP)/include/avl.h \
		$(LDAPTOP)/include/lthread.h \
	 	$(LDAPTOP)/include/ldif.h \
		$(LDAPTOP)/include/ldbm.h \
		back-ldbm.h 
