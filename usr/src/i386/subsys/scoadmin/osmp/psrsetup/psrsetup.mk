#--------------------------------------------------------------------------
# 	@(#)psrsetup.mk	1.2
#       Copyright (C) The Santa Cruz Operation, 1996.
#       This Module contains Proprietary Information of
#       The Santa Cruz Operation, and should be treated as Confidential.
#--------------------------------------------------------------------------

TOP=..
DOSMROOT=../..

include $(DOSMROOT)/make.inc.dosm
include $(TOP)/make.inc

DESTDIR=$(INSDIR)/usr/lib/scoadmin/psrsetup
DESTDIR_OBJ=$(DESTDIR)/psrsetup.obj
DESTDIR_HOOK=$(INSDIR)/usr/lib/scohelp/hooks

# Internationalization
MSGPRE=psrsetup
MODID=SCO_PSRSETUPGUI
MSGTCL=$(MSGPRE).msg.tcl
MSGTCLSRC=NLS/en/$(MSGPRE).msg
MODFILE=NLS/$(MSGPRE).mod

# The Application
APP=psrsetupGUI
HOOK=$(APP).hk
SRC=$(MSGTCL) uiProcessor.tcl objcalls.tcl main.tcl

all:	$(APP)

$(APP): $(SRC) 
	${SCRIPTSTRIP} -b -l /bin/osavtcl -o $(@) $(SRC)
	chmod +x $(APP)

$(MSGTCL): $(MSGTCLSRC)
	$(MKCATDECL) -c -i $(MODFILE) -m $(MODID) $(MSGTCLSRC)

install: $(APP) nls
	@-[ -d $(DESTDIR) ] || mkdir -p $(DESTDIR)
	@-[ -d $(DESTDIR_OBJ) ] || mkdir -p $(DESTDIR_OBJ)
	@-[ -d $(DESTDIR_HOOK) ] || mkdir -p $(DESTDIR_HOOK)
	$(INS) -f $(DESTDIR) -m 755 -u $(OWN) -g $(GRP) $(APP)
#	$(INS) -f $(DESTDIR_OBJ) -m 755 -u $(OWN) -g $(GRP) activate
	$(INS) -f $(DESTDIR_OBJ) -m 755 -u $(OWN) -g $(GRP) activate.scoadmin
#	$(INS) -f $(DESTDIR_HOOK) -m 444 -u $(OWN) -g $(GRP) $(HOOK)

nls:
	$(DONLS) -r $(DOSMROOT) -p psrsetupGUI -d psrsetup -s NLS install

clean:
	$(DONLS) -r $(ROOT) -p psrsetupGUI -d psrsetup -s NLS $@

clobber: clean
	$(DONLS) -r $(ROOT) -p psrsetupGUI -d psrsetup -s NLS $@
	rm -rf $(APP) $(MSGTCL)
