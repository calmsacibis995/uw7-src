#ident	"@(#)slapd.mk	1.11"

LDAPTOP= 	../..
CURRENT_DIR= slapd

include $(CMDRULES)
include $(LDAPTOP)/local.defs

INSDIR= 	$(USRLIB)/ldap

HDIR=		$(LDAPTOP)/include
LOCALINC=	-I$(HDIR) -I.

LDIR=		$(LDAPTOP)/usr.lib
LDLIBS=		-lldap -llber -lldbm -lavl -llthread -lldif $(THREADSLIB) \
		-lsocket -lnsl -lgen -lresolv $(LDBMLIB) -lthread

VERSIONFILE=	$(LDAPTOP)/version

OBJECTS		= main.o daemon.o connection.o search.o filter.o add.o \
		charray.o \
		attr.o entry.o config.o backend.o result.o operation.o \
		dn.o compare.o modify.o delete.o modrdn.o ch_malloc.o \
		value.o ava.o bind.o unbind.o abandon.o filterentry.o \
		phonetic.o regex.o acl.o str2filter.o aclparse.o init.o \
		detach.o repl.o lock.o \
		schema.o schemaparse.o monitor.o configinfo.o \
		pidfile.o


all: 		
		@$(MAKE) -f slapd.mk $(MAKEARGS) backendslib; \
		$(MAKE) -f slapd.mk $(MAKEARGS) slapd; \
		cd tools; \
		echo "making" $@ "in $(CURRENT_DIR)/tools/$$i..."; \
		$(MAKE) -f tools.mk $(MAKEARGS) all; 


slapd:		$(OBJECTS) version.o libbackends.a
		$(CC) -o slapd  $(LDFLAGS) -L$(LDIR) $(OBJECTS) version.o \
			libbackends.a $(LDLIBS) $(SHLIBS)


backendslib:	
		@for i in back-*; do \
			cd $$i; \
			echo "making all in  $(CURRENT_DIR)/$$i..."; \
			$(MAKE) -f $$i.mk $(MAKEARGS) all ; \
			cd ..; \
		done; \
		echo; \
		$(MAKE) -f slapd.mk libbackends.a

libbackends.a: 
	@$(RM) -rf tmp
	mkdir tmp
	for i in back-*/*.a; do \
		( \
		  cd tmp; \
		  $(AR) x ../$$i; \
		  pre=`echo $$i | sed -e 's/\/.*$$//' -e 's/back-//'`; \
		  for j in *.o; do \
			mv $$j $${pre}$$j; \
		  done; \
		  $(AR) ruv libbackends.a *.o 2>&1 | grep -v truncated; \
		  $(RM) -f *.o __.SYMDEF; \
		  echo "added backend library $$i"; \
		); \
	done
	@mv -f tmp/libbackends.a ./libbackends.a
	@$(RM) -r tmp
	@if [ ! -z "$(RANLIB)" ]; then \
		$(RANLIB) libbackends.a; \
	fi
	@ls -l libbackends.a

version.c: libbackends.a $(OBJS) $(LDIR)/liblber/liblber.so \
		$(LDIR)/libldbm/libldbm.a $(LDIR)/liblthread/liblthread.a \
		$(LDIR)/libavl/libavl.a $(LDIR)/libldif/libldif.a
	$(RM) -f $@
	echo versionfile $(VERSIONFILE) 
	(v=`cat $(VERSIONFILE)` t=`date`; sed -e "s|%WHEN%|$${t}|" \
	-e "s|%VERSION%|$${v}|" \
	< Version.c > $@)

install:	all
		[ -d $(INSDIR) ] || mkdir -p $(INSDIR)
		$(INS) -f $(INSDIR) slapd
		cd tools; \
		$(MAKE) -f tools.mk $(MAKEARGS) $@; 

clean:
	$(RM) -f *.o version.c libbackends.a 
	@for i in back-* tools; do \
		if [ -d $$i ]; then \
		echo; echo "  cd $$i; $(MAKE) -f $$i.mk $(MAKEARGS) $@"; \
		( cd $$i; $(MAKE) -f $$i.mk $@ ); \
		fi; \
	done

clobber:	clean
	$(RM) -f slapd 
	@for i in back-* tools; do \
		if [ -d $$i ]; then \
		echo; echo "  cd $$i; $(MAKE) -f $$i.mk $(MAKEARGS) $@"; \
		( cd $$i; $(MAKE) -f $$i.mk $@ ); \
		fi; \
	done

lintit:
	$(LINT) $(LINTFLAGS) $(CFLAGS) $(INCLIST) $(DEFLIST) *.c
	@for i in back-* tools; do \
		if [ -d $$i ]; then \
		echo; echo "  cd $$i; $(MAKE) -f $$i.mk $(MAKEARGS) $@"; \
		( cd $$i; $(MAKE) -f $$i.mk $@ ); \
		fi; \
	done


FRC:

#
# Header dependencies
#
abandon.o:	abandon.c \
		slap.h

acl.o:		acl.c \
		$(LDAPTOP)/include/regex.h \
		slap.h

aclparse.o:	aclparse.c \
		$(LDAPTOP)/include/regex.h \
		$(LDAPTOP)/include/portable.h \
		slap.h

add.o:		add.c \
		slap.h

attr.o:		attr.c \
		$(LDAPTOP)/include/portable.h \
		slap.h

ava.o:		ava.c \
		slap.h

backend.o:	backend.c \
		slap.h

bind.o:		bind.c \
		slap.h

ch_malloc.o:	ch_malloc.c \
		slap.h

charray.o:	charray.c \
		slap.h

compare.o:	compare.c \
		slap.h

config.o:	config.c \
		slap.h \
		$(LDAPTOP)/include/ldapconfig.h

configinfo.o:	configinfo.c \
		slap.h \
		$(LDAPTOP)/include/ldapconfig.h

connection.o:	connection.c \
		slap.h \
		$(LDAPTOP)/include/portable.h

daemon.o:	daemon.c \
		slap.h \
		$(LDAPTOP)/include/ldapconfig.h \
		$(LDAPTOP)/include/portable.h

delete.o:	delete.c \
		slap.h

detach.o:	detach.c \
		$(LDAPTOP)/include/portable.h

dn.o:		dn.c \
		slap.h \
		$(LDAPTOP)/include/portable.h

entry.o:	entry.c \
		slap.h

filter.o:	filter.c \
		slap.h

filterentry.o:	filterentry.c \
		$(LDAPTOP)/include/regex.h \
		slap.h

init.o:		init.c \
		slap.h \
		$(LDAPTOP)/include/portable.h

lock.o:		lock.c \
		slap.h \
		$(LDAPTOP)/include/portable.h

main.o:		main.c \
		slap.h \
		$(LDAPTOP)/include/ldaplog.h \
		$(LDAPTOP)/include/ldapconfig.h \
		$(LDAPTOP)/include/portable.h

modify.o:	modify.c \
		slap.h

modrdn.o:	modrdn.c \
		slap.h

monitor.o:	monitor.c \
		slap.h \
		$(LDAPTOP)/include/ldapconfig.h 

operation.o:	operation.c \
		slap.h

phonetic.o:	phonetic.c \
		slap.h 

regex.o:	regex.c \
		$(LDAPTOP)/include/regex.h

repl.o:		repl.c \
		slap.h

result.o:	result.c \
		slap.h \
		$(LDAPTOP)/include/portable.h

schema.o:	schema.c \
		slap.h

schemaparse.o:	schemaparse.c \
		slap.h

search.o:	search.c \
		slap.h \
		$(LDAPTOP)/include/ldapconfig.h 

str2filter.o:	str2filter.c \
		slap.h

unbind.o:	unbind.c \
		slap.h

value.o:	value.c \
		slap.h \
		$(LDAPTOP)/include/portable.h

pidfile.o:	pidfile.c \
		slap.h

