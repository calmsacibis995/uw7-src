include $(CMDRULES)

#	Makefile for misc

OWN = bin
GRP = bin

MAKEFILE = misc.mk



all:
	[ -d $(ROOT)/$(MACH)/usr/dt/lib/nls/msg/ja ] || mkdir -p $(ROOT)/$(MACH)/usr/dt/lib/nls/msg/ja
	[ -d $(ROOT)/$(MACH)/usr/dt/lib/nls/msg/ja ] || mkdir -p $(ROOT)/$(MACH)/usr/dt/lib/nls/msg/ja
	[ -d $(ROOT)/$(MACH)/usr/dt/bin ] || mkdir -p $(ROOT)/$(MACH)/usr/dt/bin

install: all
	$(INS) -f $(ROOT)/$(MACH)/usr/dt/lib/nls/msg/ja -m 755 -u bin -g bin $(ROOT)/usr/src/i386at/misc/usr/dt/lib/nls/msg/ja/dtpad.cat.m
	$(INS) -f $(ROOT)/$(MACH)/usr/dt/lib/nls/msg/ja -m 755 -u bin -g bin $(ROOT)/usr/src/i386at/misc/usr/dt/lib/nls/msg/ja/dtterm.cat.m
	$(INS) -f $(ROOT)/$(MACH)/usr/dt/lib -m 755 -u root -g sys $(ROOT)/usr/src/i386at/misc/usr/dt/lib/libDtTerm.a
	$(INS) -f $(ROOT)/$(MACH)/usr/dt/lib -m 755 -u bin -g bin $(ROOT)/usr/src/i386at/misc/usr/dt/lib/libDtTerm.so.1
	$(INS) -f $(ROOT)/$(MACH)/usr/dt/bin -m 755 -u root -g bin $(ROOT)/usr/src/i386at/misc/usr/dt/bin/dtterm


lintit:

#	These targets are useful but optional

clean:

clobber:

