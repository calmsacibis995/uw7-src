#ident	"@(#)niccfg.mk	20.2"

include $(CMDRULES)

SUBDIRS = help supported_nics

OWN = bin
GRP = bin

TOOLS_DIR  = $(ROOT)/$(MACH)/etc/inst/nics/tools
# 
#SCRIPTS_DIR  = $(ROOT)/$(MACH)/etc/inst/nics/scripts
RC2_DIR = $(ROOT)/$(MACH)/etc/rc2.d

LOCALE_DIR = $(ROOT)/$(MACH)/etc/inst/locale/C/menus/nics
CONFIG_DIR = $(LOCALE_DIR)/config
HELP_DIR   = $(LOCALE_DIR)/help
SUPP_DIR   = $(LOCALE_DIR)/supported_nics/help

all: clean
	$(CC) -o tools/findvt tools/findvt.c
	for d in $(SUBDIRS) ; \
	do \
		cd $$d; \
		for i in *; \
		do \
			[ -f $$i ] && $(USRBIN)/hcomp $$i ; \
		done ; \
		cd .. ; \
	done

install: all
	[ -d $(TOOLS_DIR) ]   || mkdir -p $(TOOLS_DIR)
#	[ -d $(SCRIPTS_DIR) ] || mkdir -p $(SCRIPTS_DIR)
	[ -d $(CONFIG_DIR) ]  || mkdir -p $(CONFIG_DIR)
	[ -d $(HELP_DIR) ]    || mkdir -p $(HELP_DIR)
	[ -d $(SUPP_DIR) ]    || mkdir -p $(SUPP_DIR)

	$(INS) -f $(TOOLS_DIR)  -m 0744 -u $(OWN) -g $(GRP) tools/findvt
	$(INS) -f $(USRSBIN)    -m 0744 -u $(OWN) -g $(GRP) tools/niccfg
	$(INS) -f $(TOOLS_DIR)  -m 0744 -u $(OWN) -g $(GRP) tools/nic_stty
	$(INS) -f $(RC2_DIR)    -m 0444 -u $(OWN) -g $(GRP) tools/S02NICS

	$(INS) -f $(LOCALE_DIR)  -m 0644 -u $(OWN) -g $(GRP) tools/nic_strings

	( cd $(ROOT)/usr/src/$(WORK)/cmd/niccfg/config; \
	for i in * ; \
	do \
		$(INS) -f $(CONFIG_DIR) -m 0644 -u $(OWN) -g $(GRP) $$i; \
	done ; \
	)

	( cd $(ROOT)/usr/src/$(WORK)/cmd/niccfg/help; \
	for i in *.hcf ; \
	do \
		$(INS) -f $(HELP_DIR) -m 0644 -u $(OWN) -g $(GRP) $$i; \
	done ; \
	)

	( cd $(ROOT)/usr/src/$(WORK)/cmd/niccfg/supported_nics; \
	for i in *.hcf ; \
	do \
		$(INS) -f $(SUPP_DIR) -m 0644 -u $(OWN) -g $(GRP) $$i; \
	done ; \
	)

clean:
	for d in $(SUBDIRS) ; \
	do \
		cd $$d; \
		$(RM) -f *.hcf; \
		cd ..; \
	done
	$(RM) -f tools/findvt

clobber: clean
