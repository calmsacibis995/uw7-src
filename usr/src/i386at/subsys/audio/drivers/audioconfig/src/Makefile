#==============================================================================
# Audio Configuration Manager
#------------------------------------------------------------------------------
# Makefile
#------------------------------------------------------------------------------
# @(#)	@(#)Makefile	7.5	1/9/98	15:15:41
#------------------------------------------------------------------------------
# Makefile for audioconfig
#------------------------------------------------------------------------------
# Revision History
# 1997-Jul-10, shawnm, add NLSSUBDIRS
# 1997-Jan-22, shawnm, add CONFIGFILE
# 1997-Jan-21, shawnm, don't use -m -g -u on install
# 1997-Jan-16, shawnm, added osa rules
# 1996-Dec-06, shawnm, added ui dialog files and system.tcl
# 1996-Sep-27, shawnm, created from template
#==============================================================================

AUCFGDIR = $(ROOT)/$(MACH)/usr/lib/audio/audioconfig
AUCFGPXDIR = $(ROOT)/$(MACH)/usr/lib/audio/audioconfig/px

# Tools
DOSMROOT=../../../../scoadmin
SYSADMBASE=$(DOSMROOT)/sysAdm
VTCLBASE	=	$(TOOLS)
TCL_LIBRARY=$(VTCLBASE)/usr/lib/tcl7.6/
TCLX_LIBRARY=$(VTCLBASE)/usr/lib/tclX/7.6.0
TCL=LD_LIBRARY_PATH=$(VTCLBASE)/usr/lib $(VTCLBASE)/usr/bin/tcl
RUNTCL=TCL_LIBRARY=$(TCL_LIBRARY) TCLX_LIBRARY=$(TCLX_LIBRARY) $(TCL)
MKCATDECL=$(RUNTCL) $(TOOLS)/usr/bin/mkcatdecl
GENCAT = /usr/bin/gencat
INS = $(TOOLS)/etc/install
# COPYRIGHT = $(DOSMROOT)/sysAdm/tools/hdrs/shipCopyRight.tcl
COPYRIGHT = /dev/null

# Internationalization
NLS = ./NLS
NLSSUBDIRS = en
MSGDEFAULT = en
MSGPRE = audioconfig
OSAMSGPRE = audioconfig.osa
MODID = SCO_AUDIOCONFIG
OSAMODID = SCO_AUDIOCONFIGOSA
MSGTCL = $(MSGPRE).msg.tcl
OSAMSGTCL = $(OSAMSGPRE).msg.tcl
MSGTCLSRC = $(NLS)/$(MSGDEFAULT)/$(MSGPRE).msg
OSAMSGTCLSRC = $(NLS)/$(MSGDEFAULT)/$(OSAMSGPRE).msg
MODFILE = $(NLS)/$(MSGPRE).mod
OSAMODFILE = $(NLS)/$(OSAMSGPRE).mod
MSGCATROOT = $(ROOT)/$(MACH)/usr/lib/locale

# The Application
APP = audioconfig

# The OSA
OSA = audioconfig.osa
CONFIGFILE = audioconfig.cfg

# Ordering of the source is important:
# 1) Message.tcl and intl.tcl first
# 2) main.tcl last
SRC = $(MSGTCL) 	intl.tcl \
		ui.tcl uimenu.tcl uitoolbar.tcl uilist.tcl \
		uiadd.tcl uiexamine.tcl uiremove.tcl uitest.tcl \
		system.tcl \
		sso.tcl main.tcl

# Desktop Object
OBJ = audioconfig.obj

all: $(APP) $(OSA)

$(APP): $(SRC)
	echo "#!/bin/osavtcl" > $(APP)
	echo "loadlibindex /usr/lib/sysadm.tlib" >> $(APP)
	cat $(SRC) >> $(APP)
	chmod +x $(APP)

$(MSGTCL): $(MSGTCLSRC)
	$(MKCATDECL) -c -i $(MODFILE) -m $(MODID) $(MSGTCLSRC)
	@for i in $(NLSSUBDIRS) ;\
	do \
	($(GENCAT) -X $(NLS)/$$i/$(MSGPRE).cat $(NLS)/$$i/$(MSGPRE).gen); \
	done


OSASRC = $(OSAMSGTCL) $(OSA).cdt $(OSA).procs

$(OSA): $(OSASRC)
	echo "#!/bin/osavtcl" > $(OSA)
	cat $(COPYRIGHT) >> $(OSA)
	echo "loadlibindex /usr/lib/sysadm.tlib" >> $(OSA)
	cat $(OSASRC) >> $(OSA)

$(OSAMSGTCL): $(OSAMSGTCLSRC)
	$(MKCATDECL) -c -i $(OSAMODFILE) -m $(OSAMODID) $(OSAMSGTCLSRC)
	$(GENCAT) -X $(NLS)/$(MSGDEFAULT)/$(OSAMSGPRE).cat $(NLS)/$(MSGDEFAULT)/$(OSAMSGPRE).gen


install: $(APP) $(OSA)
	@if [ ! -d $(AUCFGDIR) ]; then \
		mkdir -p $(AUCFGDIR); \
	fi
	$(INS) -f $(AUCFGDIR) $(APP)
	$(INS) -f $(AUCFGDIR) $(OSA)
	$(INS) -f $(AUCFGDIR) $(CONFIGFILE)
	$(INS) -f $(AUCFGDIR) cleanup
	$(INS) -f $(AUCFGDIR) audconfGUI.hk
	$(INS) -f $(AUCFGDIR) spacemusic.au
	@if [ ! -d $(AUCFGPXDIR) ]; then \
		mkdir -p $(AUCFGPXDIR); \
	fi
	$(INS) -f $(AUCFGPXDIR) px/add.px
	$(INS) -f $(AUCFGPXDIR) px/test.px
	@if [ ! -d $(AUCFGDIR)/$(OBJ) ]; then \
		mkdir -p $(AUCFGDIR)/$(OBJ); \
	fi
	$(INS) -f $(AUCFGDIR)/$(OBJ) activate.scoadmin
	@if [ ! -d $(AUCFGDIR)/$(OBJ)/C ]; then \
		mkdir -p $(AUCFGDIR)/$(OBJ)/C; \
	fi
	$(INS) -f $(AUCFGDIR)/$(OBJ)/C $(NLS)/$(MSGDEFAULT)/title
	@if [ ! -d $(AUCFGDIR)/$(OBJ)/en_US ]; then \
		mkdir -p $(AUCFGDIR)/$(OBJ)/en_US; \
	fi
	$(INS) -f $(AUCFGDIR)/$(OBJ)/en_US $(NLS)/$(MSGDEFAULT)/title
	@for i in $(NLSSUBDIRS) ;\
	do \
	(if [ ! -d $(MSGCATROOT)/$$i/LC_MESSAGES ]; then \
		mkdir -p $(MSGCATROOT)/$$i/LC_MESSAGES; \
	fi); \
	done

clean:
	    rm -rf *.msg.tcl $(NLS)/$(MSGDEFAULT)/*gen $(NLS)/$(MSGDEFAULT)/*cat

clobber: clean
	rm -rf $(OBJ)/$(APP)
	rm -f $(APP) $(MSGTCL) $(OSA) $(OSAMSGTCL)

