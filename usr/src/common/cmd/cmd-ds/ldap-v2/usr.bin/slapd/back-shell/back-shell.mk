# @(#)back-shell.mk	1.4

LDAPTOP=	../../..

include $(LIBRULES)
include $(LDAPTOP)/local.defs

HDIR=		$(LDAPTOP)/include
SLAPDIR=	..
LOCALINC=	-I$(HDIR) -I$(SLAPDIR)

LIBRARY = libback-shell.a
OBJECTS = init.o config.o fork.o search.o bind.o unbind.o add.o delete.o \
		modify.o modrdn.o compare.o abandon.o result.o

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

init.o: 	init.c \
		../slap.h \
		$(INC)/lber.h \
		$(INC)/ldap.h \
		$(LDAPTOP)/include/avl.h \
		$(LDAPTOP)/include/lthread.h \
		$(LDAPTOP)/include/ldif.h \
		shell.h 

config.o: 	config.c \
		../slap.h \
		$(INC)/lber.h \
		$(INC)/ldap.h \
		$(LDAPTOP)/include/avl.h \
		$(LDAPTOP)/include/lthread.h \
		$(LDAPTOP)/include/ldif.h \
		shell.h 

fork.o: 	fork.c \
		../slap.h \
		$(INC)/lber.h \
		$(INC)/ldap.h \
		$(LDAPTOP)/include/avl.h \
		$(LDAPTOP)/include/lthread.h \
		$(LDAPTOP)/include/ldif.h 

search.o: 	search.c \
		../slap.h \
		$(INC)/lber.h \
		$(INC)/ldap.h \
		$(LDAPTOP)/include/avl.h \
		$(LDAPTOP)/include/lthread.h \
		$(LDAPTOP)/include/ldif.h \
		shell.h 

bind.o: 	bind.c \
		../slap.h \
		$(INC)/lber.h \
		$(INC)/ldap.h \
		$(LDAPTOP)/include/avl.h \
		$(LDAPTOP)/include/lthread.h \
		$(LDAPTOP)/include/ldif.h \
		shell.h 

unbind.o: 	unbind.c \
		../slap.h \
		$(INC)/lber.h \
		$(INC)/ldap.h \
		$(LDAPTOP)/include/avl.h \
		$(LDAPTOP)/include/lthread.h \
		$(LDAPTOP)/include/ldif.h \
		shell.h 

add.o: 	add.c \
		../slap.h \
		$(INC)/lber.h \
		$(INC)/ldap.h \
		$(LDAPTOP)/include/avl.h \
		$(LDAPTOP)/include/lthread.h \
		 $(LDAPTOP)/include/ldif.h \
		shell.h 

delete.o: 	delete.c \
		../slap.h \
		$(INC)/lber.h \
		 $(INC)/ldap.h \
		$(LDAPTOP)/include/avl.h \
		$(LDAPTOP)/include/lthread.h \
		 $(LDAPTOP)/include/ldif.h \
		shell.h 

modify.o: 	modify.c \
		../slap.h \
		$(INC)/lber.h \
		 $(INC)/ldap.h \
		$(LDAPTOP)/include/avl.h \
		$(LDAPTOP)/include/lthread.h \
		$(LDAPTOP)/include/ldif.h \
		shell.h 

modrdn.o: 	modrdn.c \
		../slap.h \
		$(INC)/lber.h \
		 $(INC)/ldap.h \
		$(LDAPTOP)/include/avl.h \
		$(LDAPTOP)/include/lthread.h \
 		$(LDAPTOP)/include/ldif.h \
		shell.h 

compare.o: 	compare.c \
		../slap.h \
		$(INC)/lber.h \
		$(INC)/ldap.h \
		$(LDAPTOP)/include/avl.h \
		$(LDAPTOP)/include/lthread.h \
		$(LDAPTOP)/include/ldif.h \
		shell.h 

abandon.o: 	abandon.c \
		../slap.h \
		$(INC)/lber.h \
		$(INC)/ldap.h \
		$(LDAPTOP)/include/avl.h \
		$(LDAPTOP)/include/lthread.h \
		$(LDAPTOP)/include/ldif.h \
		shell.h 

result.o: 	result.c \
		../slap.h \
		$(INC)/lber.h \
		$(INC)/ldap.h \
		$(LDAPTOP)/include/avl.h \
		$(LDAPTOP)/include/lthread.h \
		$(LDAPTOP)/include/ldif.h \
		shell.h 
