#ident	"@(#)pcintf:pcintf.mk	1.1.3.2"
#	Copyright (c) 1991  Locus Computing Corporation
#	All Rights Reserved
#

include	$(CMDRULES)

PCI	= $(USR)/pci
PCIBIN	= $(PCI)/bin
PCILIB	= $(PCI)/lib
DOSMSG	= $(PCI)/dosmsg
PCILOG	= $(VAR)/spool/pcilog

DIRS	= \
	$(ETC)/rc0.d \
	$(ETC)/rc1.d \
	$(ETC)/rc2.d \
	$(USRSHARE)/lib/terminfo/v \
	$(PCI) \
	$(PCIBIN) \
	$(PCILIB) \
	$(DOSMSG) \
	$(PCILOG)

all:
	cd bridge; \
	$(MAKE) -f svr4.mk svr4eth; \
	$(MAKE) -f svr4.mk svr4232

install: all $(DIRS)
	cp bridge/svr4/eth/consvr bridge/svr4/eth/pciconsvr.ip
	cp bridge/svr4/eth/dossvr bridge/svr4/eth/pcidossvr.ip
	cp bridge/svr4/eth/dosout bridge/svr4/eth/pcidosout.ip
	cp bridge/svr4/eth/mapsvr bridge/svr4/eth/pcimapsvr.ip
	cp bridge/svr4/rs232/dossvr bridge/svr4/rs232/pcidossvr.232
	cp bridge/svr4/rs232/dosout bridge/svr4/rs232/pcidosout.232
	cp bridge/svr4/pkg_rlock/rlockshm bridge/svr4/pkg_rlock/sharectl
	$(INS) -f $(PCIBIN) support/svr4/errlogger
	$(INS) -f $(PCIBIN) support/svr4/pcistart
	$(INS) -f $(PCIBIN) support/svr4/pcistop
	$(INS) -f $(PCIBIN) support/svr4/pciprint
	$(INS) -f $(PCIBIN) support/svr4/environ
	$(INS) -f $(PCILIB) -m 644 support/svr4/vt220-pc.ti
	$(INS) -f $(USRSHARE)/lib/terminfo/v -m 644 support/svr4/vt220-pc
	$(INS) -f $(PCI) support/svr4/pcidossvr.232
	$(INS) -f $(PCI) -m 644 support/svr4/pciptys
	$(INS) -f $(ETC)/rc2.d support/svr4/S95pci
	rm -rf $(ETC)/rc0.d/K95pci
	rm -rf $(ETC)/rc1.d/K95pci
	ln $(ETC)/rc2.d/S95pci		$(ETC)/rc0.d/K95pci
	ln $(ETC)/rc2.d/S95pci		$(ETC)/rc1.d/K95pci
	$(INS) -f $(PCIBIN) bridge/svr4/eth/loadpci
	$(INS) -f $(PCIBIN) bridge/svr4/eth/pciconsvr.ip
	$(INS) -f $(PCIBIN) bridge/svr4/eth/pcidossvr.ip
	$(INS) -f $(PCIBIN) bridge/svr4/eth/pcidosout.ip
	$(INS) -f $(PCIBIN) bridge/svr4/eth/pcimapsvr.ip
	$(INS) -f $(PCIBIN) bridge/svr4/eth/pcidebug
	$(INS) -f $(DOSMSG) -m 644 bridge/svr4/eth/En.lmf
	$(INS) -f $(PCILIB) -m 644 bridge/svr4/pkg_lcs/pc437.lcs
	$(INS) -f $(PCILIB) -m 644 bridge/svr4/pkg_lcs/pc850.lcs
	$(INS) -f $(PCILIB) -m 644 bridge/svr4/pkg_lcs/8859.lcs
	$(INS) -f $(PCILIB) -m 644 bridge/svr4/pkg_lcs/pc860.lcs
	$(INS) -f $(PCILIB) -m 644 bridge/svr4/pkg_lcs/pc863.lcs
	$(INS) -f $(PCILIB) -m 644 bridge/svr4/pkg_lcs/pc865.lcs
	$(INS) -f $(PCIBIN) bridge/svr4/pkg_lmf/lmfmsg
	$(INS) -f $(PCIBIN) bridge/svr4/rs232/pcidossvr.232
	$(INS) -f $(PCIBIN) bridge/svr4/rs232/pcidosout.232
	$(INS) -f $(PCIBIN) bridge/svr4/pkg_rlock/sharectl
	$(INS) -f $(USRBIN) bridge/svr4/util/charconv
	rm -rf $(USRBIN)/dos2unix $(USRBIN)/unix2dos
	ln $(USRBIN)/charconv				$(USRBIN)/dos2unix
	ln $(USRBIN)/charconv				$(USRBIN)/unix2dos

$(DIRS):
	-mkdir -p $@
	$(CH)chmod 0755 $@
	$(CH)chown bin $@
	$(CH)chgrp bin $@

clean:
	rm -f bridge/svr4/pkg_lmf/*.o	
	rm -f bridge/svr4/pkg_lcs/*.o	
	rm -f bridge/svr4/pkg_lockset/*.o	
	rm -f bridge/svr4/pkg_rlock/*.o	
	rm -f bridge/svr4/util/*.o	
	rm -f bridge/svr4/eth/*.o	
	rm -f bridge/svr4/rs232/*.o	

clobber: clean
	-rm -rf bridge/svr4

lintit:
	cd bridge; $(MAKE) -f svr4.mk lintit
