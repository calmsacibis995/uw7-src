#ident "@(#)libldap.mk	1.7"

LDAPTOP=	../../..

include $(CMDRULES)
include $(LDAPTOP)/local.defs

INSDIR=		$(USRBIN)
OWN=		bin
GRP=		bin

HDIR=		$(LDAPTOP)/include
LOCALINC=	-I$(HDIR) -I. -I$(LDAPTOP)/usr.lib/libldap

LDIR=		$(LDAPTOP)/usr.lib
LDLIBS=		-lldap -llber -lsocket -lnsl -lresolv -llog

TESTS=		ltest 
SOURCES=	ltest.c 


all:	$(TESTS)

$(TESTS):	$$@.o $(LDIR)/libldap.so $(LDIR)/liblber.so
	$(CC) -o $@ $(LDFLAGS) $@.o -L$(LDIR) $(LDLIBS) $(SHLIBS)

$(LDIR)/libldap.so $(LDIR)/liblber.so:	
	cd $(LDIR); make -f usr.lib.mk all

install: 

clean:
	$(RM) -f *.o 

clobber:	clean
	$(RM) -f $(TESTS)

lintit:
	$(LINT) $(LINTFLAGS) $(CFLAGS) $(INCLIST) $(DEFLIST) $(SOURCES)

#
# Header dependencies
#
bind.o:		bind.c \
		$(INC)/lber.h \
		$(INC)/ldap.h

open.o:		open.c \
		$(INC)/lber.h \
		$(INC)/ldap.h \
		ldap-int.h

result.o:	result.c \
		$(LDAPTOP)/include/portable.h \
		$(INC)/lber.h \
		$(INC)/ldap.h 
		ldap-int.h

error.o:	error.c \
		$(INC)/lber.h \
		$(INC)/ldap.h

compare.o:	compare.c \
		$(INC)/lber.h \
		$(INC)/ldap.h \
		ldap-int.h

search.o:	search.c 
		$(INC)/lber.h \
		$(INC)/ldap.h \
		ldap-int.h

modify.o:	modify.c \
		$(INC)/lber.h \
		$(INC)/ldap.h \
		ldap-int.h

add.o:		add.c \
		$(INC)/lber.h \
		$(INC)/ldap.h \
		ldap-int.h

modrdn.o:	modrdn.c \
		$(INC)/lber.h \
		$(INC)/ldap.h \
		ldap-int.h

delete.o:	delete.c \
		$(INC)/lber.h \
		$(INC)/ldap.h \
		ldap-int.h

abandon.o:	abandon.c \
		$(INC)/lber.h \
		$(INC)/ldap.h \
		ldap-int.h

ufn.o:		ufn.c \
		$(INC)/lber.h \
		$(INC)/ldap.h 

cache.o:	cache.c 
		$(INC)/lber.h \
		$(INC)/ldap.h \
		ldap-int.h

getfilter.o:	getfilter.c \
		$(INC)/lber.h \
		$(INC)/ldap.h \
		$(LDAPTOP)/include/regex.h

regex.o:	regex.c \
		$(LDAPTOP)/include/portable.h

sbind.o:	sbind.c \
		$(INC)/lber.h \
		$(INC)/ldap.h \
		ldap-int.h

kbind.o:	kbind.c \
		$(INC)/lber.h \
		$(INC)/ldap.h \
		ldap-int.h

unbind.o:	unbind.c \
		$(INC)/lber.h \
		$(INC)/ldap.h \
		ldap-int.h

friendly.o:	friendly.c \
		$(INC)/lber.h \
		$(INC)/ldap.h 

cldap.o:	cldap.c \
		$(INC)/lber.h \
		$(INC)/ldap.h \
		ldap-int.h

free.o:		free.c \
		$(INC)/lber.h \
		$(INC)/ldap.h 

disptmpl.o:	disptmpl.c \
		$(INC)/lber.h \
		$(INC)/ldap.h \
		$(LDAPTOP)/include/disptmpl.h

srchpref.o:	srchpref.c \
		$(INC)/lber.h \
		$(INC)/ldap.h \
		$(LDAPTOP)/include/srchpref.h

dsparse.o:	dsparse.c \
		$(INC)/lber.h \
		$(INC)/ldap.h 

sort.o:		sort.c \
		$(INC)/lber.h \
		$(INC)/ldap.h 

getdn.o:	getdn.c \
		$(INC)/lber.h \
		$(INC)/ldap.h \
		ldap-int.h

getentry.o:	getentry.c \
		$(INC)/lber.h \
		$(INC)/ldap.h 

getattr.o:	getattr.c \
		$(INC)/lber.h \
		$(INC)/ldap.h \
		ldap-int.h

getvalues.o:	getvalues.c \
		$(INC)/lber.h \
		$(INC)/ldap.h 

addentry.o:	addentry.c \
		$(INC)/lber.h \
		$(INC)/ldap.h 

request.o:	request.c \
		$(LDAPTOP)/include/portable.h \
		$(INC)/lber.h \
		$(INC)/ldap.h \
		ldap-int.h

getdxbyname.o:	getdxbyname.c

os-ip.o:	os-ip.c \
		$(LDAPTOP)/include/portable.h \
		$(INC)/lber.h \
		$(INC)/ldap.h

url.o:		url.c \
		$(INC)/lber.h \
		$(INC)/ldap.h \
		ldap-int.h

charset.o:	charset.c
