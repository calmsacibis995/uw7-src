#ident	"@(#)libldap.mk	1.7"

DEVDEF= -DFILTERFILE="\"etc/ldapfilter.conf\"" \
        -DTEMPLATEFILE="\"etc/ldaptemplates.conf\""

LDAPTOP=	../..

include $(LIBRULES)
include $(LDAPTOP)/local.defs

INSDIR=		$(USRLIB)

HDIR=		$(LDAPTOP)/include
LOCALINC=	-I$(HDIR) 

.c.o:
	$(CC) $(CFLAGS) $(PICFLAGS) $(INCLIST) $(DEFLIST) -c $<


OBJECTS=   	abandon.o add.o addentry.o bind.o cache.o \
		compare.o defaults.o delete.o disptmpl.o \
		dsparse.o error.o free.o \
		friendly.o getattr.o getdn.o getdxbyname.o getentry.o \
		getfilter.o getpass.o getvalues.o modify.o modrdn.o \
		open.o option.o \
		os-ip.o request.o result.o search.o srchpref.o \
		regex.o sbind.o sort.o unbind.o url.o 

LIBRARY=	libldap.so

all:	$(LIBRARY)

$(LIBRARY):	$(OBJECTS)
	$(CC) -G -dy -ztext -o $@ $(OBJECTS) ../liblog.a; \
	$(RM) -f ../$@; \
	$(LN) -s libldap/$@ ../$@;

install: all
	[ -d $(INSDIR) ] || mkdir -p $(INSDIR)
	$(INS) -f $(INSDIR) $(LIBRARY)

clean:
	$(RM) -f *.o 

clobber:	clean
	$(RM) -f $(LIBRARY) ../$(LIBRARY)

lintit:
	$(LINT) $(LINTFLAGS) $(CFLAGS) $(INCLIST) $(DEFLIST) *.c

FRC:

#
# Header dependencies
#
abandon.o:	abandon.c \
		$(INC)/ldap.h \
		$(INC)/lber.h \
		ldap-int.h

add.o:		add.c \
		$(INC)/ldap.h \
		$(INC)/lber.h \
		ldap-int.h

addentry.o:	addentry.c \
		$(INC)/ldap.h \
		$(INC)/lber.h 

bind.o:		bind.c \
		$(INC)/ldap.h \
		$(INC)/lber.h 

cache.o:	cache.c \
		$(INC)/ldap.h \
		$(INC)/lber.h \
		ldap-int.h

charset.o:	charset.c \
		$(INC)/ldap.h \
		$(INC)/lber.h \
		ldap-int.h

compare.o:	compare.c \
		$(INC)/ldap.h \
		$(INC)/lber.h \
		ldap-int.h

defaults.o:	defaults.c

delete.o:	delete.c \
		$(INC)/ldap.h \
		$(INC)/lber.h \
		ldap-int.h

disptmpl.o:	disptmpl.c \
		$(INC)/ldap.h \
		$(INC)/lber.h \
		$(LDAPTOP)/include/disptmpl.h

dsparse.o:	dsparse.c \
		$(INC)/ldap.h \
		$(INC)/lber.h 

error.o:	error.c \
		$(INC)/ldap.h \
		$(INC)/lber.h 

free.o:		free.c \
		$(INC)/ldap.h \
		$(INC)/lber.h 

friendly.o:	friendly.c \
		$(INC)/ldap.h \
		$(INC)/lber.h 

getattr.o:	getattr.c \
		$(INC)/ldap.h \
		$(INC)/lber.h \
		ldap-int.h

getdn.o:	getdn.c \
		$(INC)/ldap.h \
		$(INC)/lber.h 

getdxbyname.o:	getdxbyname.c \
		$(INC)/ldap.h \
		$(INC)/lber.h \
		ldap-int.h

getentry.o:	getentry.c \
		$(INC)/ldap.h \
		$(INC)/lber.h 

getfilter.o:	getfilter.c \
		$(INC)/ldap.h \
		$(INC)/lber.h \
		$(LDAPTOP)/include/regex.h

getpass.o:	getpass.c

getvalues.o:	getvalues.c \
		$(INC)/ldap.h \
		$(INC)/lber.h 

modify.o:	modify.c \
		$(INC)/ldap.h \
		$(INC)/lber.h \
		ldap-int.h

modrdn.o:	modrdn.c \
		$(INC)/ldap.h \
		$(INC)/lber.h \
		ldap-int.h

open.o:		open.c \
		$(INC)/ldap.h \
		$(INC)/lber.h \
		ldap-int.h

option.o:	option.c \
		$(INC)/ldap.h \
		$(INC)/lber.h \
		ldap-int.h

os-ip.o:	os-ip.c \
		$(INC)/ldap.h \
		$(INC)/lber.h \
		$(LDAPTOP)/include/portable.h

regex.o:	regex.c \
		$(LDAPTOP)/include/portable.h \
		$(LDAPTOP)/include/regex.h

request.o:	request.c \
		$(INC)/ldap.h \
		$(INC)/lber.h \
		ldap-int.h

result.o:	result.c \
		$(INC)/ldap.h \
		$(INC)/lber.h \
		ldap-int.h

sbind.o:	sbind.c \
		$(INC)/ldap.h \
		$(INC)/lber.h \
		ldap-int.h

search.o:	search.c \
		$(INC)/ldap.h \
		$(INC)/lber.h \
		ldap-int.h

sort.o:		sort.c \
		$(INC)/ldap.h \
		$(INC)/lber.h 

srchpref.o:	srchpref.c \
		$(INC)/ldap.h \
		$(INC)/lber.h \
		srchpref.h

unbind.o:	unbind.c \
		$(INC)/ldap.h \
		$(INC)/lber.h \
		ldap-int.h

url.o:		url.c \
		$(INC)/ldap.h \
		$(INC)/lber.h \
		ldap-int.h

