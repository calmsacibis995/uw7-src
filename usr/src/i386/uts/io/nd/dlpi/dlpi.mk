#ident "@(#)dlpi.mk	26.3"
#ident "$Header$"

#
#	Copyright (C) The Santa Cruz Operation, 1996
#	This Module contains Proprietary Information of
#	The Santa Cruz Operation, and should be treated as Confidential.
#

#
# compilation flags of note:
# ODT_TBIRD     - largly obsolete, for historical reference
# UNIXWARE_TCP  - handle binds and ioctls specific to UnixWare 2.1 tcp stack
# BUILTINFILTER - don't call symbols resolved in pf driver -- allows standalone
#                 dlpi module (also see $interface line in Master file)
#

include $(UTSRULES)

MAKEFILE = dlpi.mk
KBASE = ../../..

BASECFGDIR = dlpibase.cf
BASEDRV = $(BASECFGDIR)/Driver.o
DLPICFGDIR = dlpi.cf
DLPIDRV = $(DLPICFGDIR)/Driver.o
NETXCFGDIR = dlpi.netX.cf
NETXDRV = $(NETXCFGDIR)/Driver.o
NETX_DIR = $(ROOT)/$(MACH)/etc/inst/nd/netX

BASEOBJ=Nonconform.o Nonconform-memory.o Nonconform-file.o
NETXOBJ=NetX.o
OBJS=	main.o dlpi.o mdi.o util.o mdiioctl.o lliioctl.o lib.o \
	llc.o dlpidata.o sr.o bind.o \
	ether.o token.o fddi.o mdilib.o mditx.o txmon.o isdn.o filter.o
LOBJS=	main.L dlpi.L mdi.L util.L mdiioctl.L lliioctl.L lib.L \
	llc.L dlpidata.L sr.L bind.L \
	ether.L token.L fddi.L mdilib.L mditx.L txmon.L isdn.L filter.L

.SUFFIXES:.c .i .L .asm

.c.i:
	$(CC) $(CFLAGS) -P $<

.c.L:
	$(LINT) $(LINTFLAGS) $(DEFLIST) $(INCLIST) $< >$@

.c.asm:
	$(CC) $(CFLAGS) -S $<

all: $(DLPIDRV) $(NETXDRV) $(BASEDRV)

$(DLPIDRV): $(OBJS)
	$(LD) -r -o $(DLPIDRV) $(OBJS)

$(BASEDRV): $(BASEOBJ)
	$(LD) -r -o $(BASEDRV) $(BASEOBJ)

$(NETXDRV): $(NETXOBJ)
	$(LD) -r -o $(NETXDRV) $(NETXOBJ)

$(OBJS): ../sys/dlpimod.h ../sys/mdi.h ./include.h dlpi.mk ./prototype.h

clean:
	rm -f $(OBJS) $(SOBJS) $(NETXOBJ) $(BASEOBJ) *.i *.L tags mdilib_t

clobber: clean
	rm -f $(DLPICFGDIR)/Driver.o
	rm -f $(NETXCFGDIR)/Driver.o
	rm -f $(BASECFGDIR)/Driver.o
	(cd $(DLPICFGDIR); $(IDINSTALL) -R$(CONF) -d -e dlpi)
	(cd $(BASECFGDIR); $(IDINSTALL) -R$(CONF) -d -e dlpibase)
	(cd $(NETXCFGDIR); $(IDINSTALL) -R$(CONF) -d -e net0)

lintit:	$(LOBJS)

install: all
	[ -d $(NETX_DIR) ] || mkdir -p $(NETX_DIR)
	(cd $(DLPICFGDIR); $(IDINSTALL) -R$(CONF) -M dlpi)
	(cd $(BASECFGDIR); $(IDINSTALL) -R$(CONF) -M dlpibase)
	(cd $(NETXCFGDIR); \
	mkdir -p .netX.$$$$; \
	$(CP) * .netX.$$$$; \
	cd .netX.$$$$; \
	$(SED) -e 's/XXXX/net0/g' -e 's/YYYY/NET0/g' Master > Master+; \
	mv -f Master+ Master; \
	$(SED) -e 's/XXXX/net0/g' -e 's/YYYY/NET0/g' System > System+; \
	mv -f System+ System; \
	$(SED) -e 's/XXXX/net0/g' -e 's/YYYY/NET0/g' Space.c > Space.c+; \
	mv -f Space.c+ Space.c; \
	$(SED) -e 's/XXXX/net0/g' -e 's/YYYY/NET0/g' Node > Node+; \
	mv -f Node+ Node; \
	$(SED) -e 's/XXXX/net0/g' -e 's/YYYY/NET0/g' Autotune > Autotune+; \
	mv -f Autotune+ Autotune; \
	$(SED) -e 's/XXXX/net0/g' -e 's/YYYY/NET0/g' Mtune > Mtune+; \
	mv -f Mtune+ Mtune; \
	$(SED) -e 's/XXXX/net0/g' -e 's/YYYY/NET0/g' Dtune > Dtune+; \
	mv -f Dtune+ Dtune; \
	$(IDINSTALL) -R$(CONF) -M net0; \
	cd ..; \
	rm -rf .netX.$$$$)
	$(INS) -f $(NETX_DIR) -m 0555 -u $(OWN) -g $(GRP) $(NETXCFGDIR)/Driver.o
	for i in $(NETXCFGDIR)/Master $(NETXCFGDIR)/Node $(NETXCFGDIR)/Space.c $(NETXCFGDIR)/System $(NETXCFGDIR)/Mtune $(NETXCFGDIR)/Autotune $(NETXCFGDIR)/Dtune ; do \
		($(INS) -f $(NETX_DIR) -m 0644 -u $(OWN) -g $(GRP) $$i) \
	done

nmlist:
	nm -e Driver.o | grep -v static | grep .text | sed 's/[ 	].*//'| more

forum:
	[ -d $(FORUMDIR) ] || mkdir -p $(FORUMDIR)
	cp -r -f $(DLPICFGDIR) $(FORUMDIR)
	cp -r -f $(NETXCFGDIR) $(FORUMDIR)
