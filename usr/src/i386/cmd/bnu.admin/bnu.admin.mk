#ident	"@(#)bnu.admin.mk	1.2"
#ident	"$Header$"

#	This makefile makes appropriate directories
#	and copies the simple admin shells into
#	the uucpmgmt directory
#

include $(CMDRULES)

OWN	=	root
GRP	=	sys
PKG	=	$(VAR)/sadm/pkg
PKGINST	=	oam
PKGBNU	=	$(PKG)/$(PKGINST)
PKGSAV	=	$(PKGBNU)/save
PKGMI	=	$(PKGSAV)/intf_install
OAMBASE	=	$(USRSADM)/sysadm
ADDONS	=	$(OAMBASE)/add-ons
BNU	=	$(ADDONS)/$(PKGINST)
NETSERV	=	$(ADDONS)/$(PKGINST)/netservices
BASIC_NET =	$(ADDONS)/$(PKGINST)/netservices/basic_networking
BIN	=	$(OAMBASE)/bin
DEVICE	=	$(BASIC_NET)/devices
DEVADD	=	$(DEVICE)/add
DEVLIST	=	$(DEVICE)/list
DEVRM	=	$(DEVICE)/remove
POLLING	=	$(BASIC_NET)/polling
POLLADD	=	$(POLLING)/add
POLLIST	=	$(POLLING)/list
POLLRM	=	$(POLLING)/remove
SYS	=	$(BASIC_NET)/systems
SYSADD	=	$(SYS)/add
SYSLIST	=	$(SYS)/list
SYSRM	=	$(SYS)/remove
SET	=	$(BASIC_NET)/setup
SETADD	=	$(SET)/add

LINK = ln

DIRS=\
	$(ADDONS) \
	$(BNU) \
	$(NETSERV) \
	$(BASIC_NET) \
	$(DEVICE) \
	$(DEVADD) \
	$(DEVLIST) \
	$(DEVRM) \
	$(POLLING) \
	$(POLLADD) \
	$(POLLIST) \
	$(POLLRM) \
	$(SET) \
	$(SETADD) \
	$(SYS) \
	$(SYSADD) \
	$(SYSLIST) \
	$(SYSRM) \
	$(PKG) \
	$(PKGBNU) \
	$(PKGSAV) \
	$(PKGMI) \
	$(BIN)

all:

install: all $(DIRS)
	-@rm -fr tmp ; mkdir tmp
	$(INS) -f $(PKGMI) -m 0644 -u $(OWN) -g $(GRP) netserv.mi
	$(INS) -f $(PKGMI) -m 0644 -u $(OWN) -g $(GRP) bnu.mi
	$(INS) -f $(PKGMI) -m 0644 -u $(OWN) -g $(GRP) bnudevices.mi
	$(INS) -f $(PKGMI) -m 0644 -u $(OWN) -g $(GRP) polling.mi
	$(INS) -f $(PKGMI) -m 0644 -u $(OWN) -g $(GRP) systems.mi
	#
	# $(INS) -f $(PKGMI) -m 0644 -u $(OWN) -g $(GRP) setup.mi
	#
	rm -f tmp/Help ; $(CP) Helpbnu tmp/Help ; \
	$(INS) -f $(BASIC_NET) -m 0644 -u $(OWN) -g $(GRP) tmp/Help
	rm -f tmp/Help ; $(CP) Helpdev tmp/Help ; \
	$(INS) -f $(DEVICE) -m 0755 -u $(OWN) -g $(GRP) tmp/Help
	rm -f tmp/Help ; $(CP) Helpdev_a tmp/Help ; \
	$(INS) -f $(DEVADD) -m 0755 -u $(OWN) -g $(GRP) tmp/Help
	$(INS) -f $(DEVADD) -m 0755 -u $(OWN) -g $(GRP) Menu.devtype
	$(INS) -f $(DEVADD) -m 0755 -u $(OWN) -g $(GRP) Menu.modtype
	$(INS) -f $(DEVADD) -m 0755 -u $(OWN) -g $(GRP) Form.acudev
	$(INS) -f $(DEVADD) -m 0755 -u $(OWN) -g $(GRP) Form.acudev1
	$(INS) -f $(DEVADD) -m 0755 -u $(OWN) -g $(GRP) Form.acudev2
	$(INS) -f $(DEVADD) -m 0755 -u $(OWN) -g $(GRP) Form.acudev3
	$(INS) -f $(DEVADD) -m 0755 -u $(OWN) -g $(GRP) Form.acudev4
	$(INS) -f $(DEVADD) -m 0755 -u $(OWN) -g $(GRP) Form.acudev5
	$(INS) -f $(DEVADD) -m 0755 -u $(OWN) -g $(GRP) Form.adddev
	$(INS) -f $(DEVADD) -m 0755 -u $(OWN) -g $(GRP) Form.dirdev
	$(INS) -f $(DEVADD) -m 0755 -u $(OWN) -g $(GRP) Form.othdev
	$(INS) -f $(DEVADD) -m 0755 -u $(OWN) -g $(GRP) Form.streams
	$(INS) -f $(DEVADD) -m 0755 -u $(OWN) -g $(GRP) Form.tlisdev
	$(INS) -f $(DEVADD) -m 0755 -u $(OWN) -g $(GRP) Form.cf_adev
	$(INS) -f $(DEVADD) -m 0755 -u $(OWN) -g $(GRP) Menu.ports
	$(INS) -f $(DEVADD) -m 0755 -u $(OWN) -g $(GRP) Text.aok
	$(INS) -f $(DEVADD) -m 0755 -u $(OWN) -g $(GRP) Text.anok
	#
	rm -f tmp/Help ; $(CP) Helpdev_l tmp/Help ;\
	$(INS) -f $(DEVLIST) -m 0755 -u $(OWN) -g $(GRP) tmp/Help
	$(INS) -f $(DEVLIST) -m 0755 -u $(OWN) -g $(GRP) Menu.lsdev
	$(INS) -f $(DEVLIST) -m 0755 -u $(OWN) -g $(GRP) Menu.lsdev_1
	$(INS) -f $(DEVLIST) -m 0755 -u $(OWN) -g $(GRP) Menu.lsdev_2
	$(INS) -f $(DEVLIST) -m 0755 -u $(OWN) -g $(GRP) Menu.lsdev_all
	rm -f $(DEVLIST)/Menu.ports
	-$(LINK) $(DEVADD)/Menu.ports $(DEVLIST)/Menu.ports
	$(INS) -f $(DEVLIST) -m 0755 -u $(OWN) -g $(GRP) Form.listdev
	#
	rm -f tmp/Help ; $(CP) Helpdev_r tmp/Help ;\
	$(INS) -f $(DEVRM) -m 0755 -u $(OWN) -g $(GRP) tmp/Help
	$(INS) -f $(DEVRM) -m 0755 -u $(OWN) -g $(GRP) Menu.rmdev
	$(INS) -f $(DEVRM) -m 0755 -u $(OWN) -g $(GRP) Menu.rmdev_1
	$(INS) -f $(DEVRM) -m 0755 -u $(OWN) -g $(GRP) Menu.rmdev_2
	$(INS) -f $(DEVRM) -m 0755 -u $(OWN) -g $(GRP) Menu.rmdev_all
	rm -f $(DEVRM)/Menu.ports
	-$(LINK) $(DEVADD)/Menu.ports $(DEVRM)/Menu.ports
	$(INS) -f $(DEVRM) -m 0755 -u $(OWN) -g $(GRP) Form.rmdev
	$(INS) -f $(DEVRM) -m 0755 -u $(OWN) -g $(GRP) Form.cf_rdev
	$(INS) -f $(DEVRM) -m 0755 -u $(OWN) -g $(GRP) Text.rmnok
	$(INS) -f $(DEVRM) -m 0755 -u $(OWN) -g $(GRP) Text.rmok
	#
	rm -f tmp/Help ; $(CP) Helpol tmp/Help ;\
	$(INS) -f $(POLLING) -m 0755 -u $(OWN) -g $(GRP) tmp/Help
	rm -f tmp/Help ; $(CP) Helpol_a tmp/Help ;\
	$(INS) -f $(POLLADD) -m 0755 -u $(OWN) -g $(GRP) tmp/Help
	$(INS) -f $(POLLADD) -m 0755 -u $(OWN) -g $(GRP) Form.addpoll
	$(INS) -f $(POLLADD) -m 0755 -u $(OWN) -g $(GRP) Menu.sysname
	$(INS) -f $(POLLADD) -m 0755 -u $(OWN) -g $(GRP) Form.cf_apoll
	$(INS) -f $(POLLADD) -m 0755 -u $(OWN) -g $(GRP) Text.aok
	$(INS) -f $(POLLADD) -m 0755 -u $(OWN) -g $(GRP) Text.anok
	rm -f tmp/Help ; $(CP) Helpol_l tmp/Help ;\
	$(INS) -f $(POLLIST) -m 0755 -u $(OWN) -g $(GRP) tmp/Help
	$(INS) -f $(POLLIST) -m 0755 -u $(OWN) -g $(GRP) Form.listpoll
	$(INS) -f $(POLLIST) -m 0755 -u $(OWN) -g $(GRP) Menu.lspoll
	$(INS) -f $(POLLIST) -m 0755 -u $(OWN) -g $(GRP) Menu.lspoll_a
	$(INS) -f $(POLLIST) -m 0755 -u $(OWN) -g $(GRP) Menu.psysname
	rm -f tmp/Help ; $(CP) Helpol_r tmp/Help ;\
	$(INS) -f $(POLLRM) -m 0755 -u $(OWN) -g $(GRP) tmp/Help
	$(INS) -f $(POLLRM) -m 0755 -u $(OWN) -g $(GRP) Menu.rmpoll
	$(INS) -f $(POLLRM) -m 0755 -u $(OWN) -g $(GRP) Menu.rmpoll_a
	$(INS) -f $(POLLRM) -m 0755 -u $(OWN) -g $(GRP) Form.cf_rpoll
	$(INS) -f $(POLLRM) -m 0755 -u $(OWN) -g $(GRP) Text.rmnok
	$(INS) -f $(POLLRM) -m 0755 -u $(OWN) -g $(GRP) Text.rmok
	$(INS) -f $(POLLRM) -m 0755 -u $(OWN) -g $(GRP) Form.rmpoll
	rm -f $(POLLRM)/Menu.psysname
	-$(LINK) $(POLLIST)/Menu.psysname $(POLLRM)/Menu.psysname
	#
	rm -f tmp/Help ; $(CP) Helsys tmp/Help ;\
	$(INS) -f $(SYS) -m 0755 -u $(OWN) -g $(GRP) tmp/Help
	rm -f tmp/Help ; $(CP) Helsys_a tmp/Help ;\
	$(INS) -f $(SYSADD) -m 0755 -u $(OWN) -g $(GRP) tmp/Help
	$(INS) -f $(SYSADD) -m 0755 -u $(OWN) -g $(GRP) Form.acusys
	$(INS) -f $(SYSADD) -m 0755 -u $(OWN) -g $(GRP) Form.addsys
	$(INS) -f $(SYSADD) -m 0755 -u $(OWN) -g $(GRP) Menu.devname
	$(INS) -f $(SYSADD) -m 0755 -u $(OWN) -g $(GRP) Form.dirsys
	$(INS) -f $(SYSADD) -m 0755 -u $(OWN) -g $(GRP) Form.othsys
	$(INS) -f $(SYSADD) -m 0755 -u $(OWN) -g $(GRP) Form.tlissys
	$(INS) -f $(SYSADD) -m 0755 -u $(OWN) -g $(GRP) Form.cf_asys
	$(INS) -f $(SYSADD) -m 0755 -u $(OWN) -g $(GRP) Text.aok
	$(INS) -f $(SYSADD) -m 0755 -u $(OWN) -g $(GRP) Text.anok
	rm -f $(DEVRM)/Menu.devname
	-$(LINK) $(SYSADD)/Menu.devname $(DEVRM)/Menu.devname
	rm -f tmp/Help ; $(CP) Helsys_l tmp/Help ;\
	$(INS) -f $(SYSLIST) -m 0755 -u $(OWN) -g $(GRP) tmp/Help
	$(INS) -f $(SYSLIST) -m 0755 -u $(OWN) -g $(GRP) Menu.lssys
	$(INS) -f $(SYSLIST) -m 0755 -u $(OWN) -g $(GRP) Menu.lssys_all
	$(INS) -f $(SYSLIST) -m 0755 -u $(OWN) -g $(GRP) Form.listsys
	rm -f $(SYSLIST)/Menu.sysname
	-$(LINK) $(POLLADD)/Menu.sysname $(SYSLIST)/Menu.sysname
	rm -f tmp/Help ; $(CP) Helsys_r tmp/Help ;\
	$(INS) -f $(SYSRM) -m 0755 -u $(OWN) -g $(GRP) tmp/Help
	$(INS) -f $(SYSRM) -m 0755 -u $(OWN) -g $(GRP) Menu.rmsys
	$(INS) -f $(SYSRM) -m 0755 -u $(OWN) -g $(GRP) Menu.rmsys_all
	$(INS) -f $(SYSRM) -m 0755 -u $(OWN) -g $(GRP) Form.cf_rsys
	$(INS) -f $(SYSRM) -m 0755 -u $(OWN) -g $(GRP) Form.rmsys
	$(INS) -f $(SYSRM) -m 0755 -u $(OWN) -g $(GRP) Text.rmnok
	$(INS) -f $(SYSRM) -m 0755 -u $(OWN) -g $(GRP) Text.rmok
	rm -f $(SYSRM)/Menu.sysname
	-$(LINK) $(POLLADD)/Menu.sysname $(SYSRM)/Menu.sysname
	rm -f tmp/Help ; $(CP) Helpset tmp/Help ;\
	$(INS) -f $(SET) -m 0755 -u $(OWN) -g $(GRP) tmp/Help
	rm -f tmp/Help ; $(CP) Helpset_a tmp/Help ;\
	$(INS) -f $(SETADD) -m 0755 -u $(OWN) -g $(GRP) tmp/Help
	$(INS) -f $(SETADD) -m 0755 -u $(OWN) -g $(GRP) Menu.setup
	#
	$(INS) -f $(BIN) -m 0755 -u $(OWN) -g $(GRP) bnuset
	$(INS) -f $(BIN) -m 0755 -u $(OWN) -g $(GRP) delentry
	$(INS) -f $(BIN) -m 0755 -u $(OWN) -g $(GRP) validhour
	$(INS) -f $(BIN) -m 0755 -u $(OWN) -g $(GRP) validls
	$(INS) -f $(BIN) -m 0755 -u $(OWN) -g $(GRP) validname
	$(INS) -f $(BIN) -m 0755 -u $(OWN) -g $(GRP) validnetaddr
	$(INS) -f $(BIN) -m 0755 -u $(OWN) -g $(GRP) validphone
	$(INS) -f $(BIN) -m 0755 -u $(OWN) -g $(GRP) validport
	$(INS) -f $(BIN) -m 0755 -u $(OWN) -g $(GRP) validsys

$(DIRS):
	mkdir $@
	$(CH)chmod 0755 $@
	$(CH)chgrp $(GRP) $@
	$(CH)chown $(OWN) $@

clean:

clobber:
	-rm -rf tmp

lintit:

size:

strip:
