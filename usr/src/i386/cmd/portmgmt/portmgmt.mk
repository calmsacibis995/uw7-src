#ident	"@(#)portmgmt.mk	1.2"
#ident "$Header$"

include $(CMDRULES)

OAMBASE=$(USRSADM)/sysadm
BINDIR = $(OAMBASE)/bin
PORTSDIR = $(OAMBASE)/menu/ports
SHDIR = ./bin

DIRS = port_monitors port_services tty_settings port_quick

O_SHFILES = \
	ckbaud \
	ckwcount \
	ckfile \
	findpmtype \
	lsopts \
	modifypm \
	pmadmopts \
	pmgetpid \
	psmod \
	pstest \
	sacopts \
	settinglist \
	tmopts \
	uniq_pmtag \
	uniq_svctag \
	uniq_label

all: $(O_SHFILES) 
	 @for i in $(DIRS) ;\
	do \
		echo "\tcd $$i && $(MAKE) -f *.mk $(MAKEARGS) $@" ;\
		if cd $$i ;\
		then \
			$(MAKE) -f *.mk $(MAKEARGS) $@ ;\
			cd .. ;\
		fi ;\
	done

install: all $(PORTSDIR) 
	$(INS) -m 644 -g bin -u bin -f $(PORTSDIR) ports.menu
	$(INS) -m 644 -g bin -u bin -f $(PORTSDIR) Help
	@for i in $(DIRS) ;\
	do \
		echo "\tcd $$i && $(MAKE) -f *.mk $(MAKEARGS) $@" ;\
		if cd $$i ;\
		then \
			$(MAKE) -f *.mk $(MAKEARGS) $@ ;\
			cd .. ;\
		fi ;\
	done
#sh scripts
	for i in $(O_SHFILES) ;\
	do \
		$(INS) -m 755 -g bin -u bin -f $(BINDIR) $(SHDIR)/$$i ;\
	done

$(O_SHFILES):
	cp $(SHDIR)/$(@).sh $(SHDIR)/$(@)

$(PORTSDIR):
	if [ ! -d `dirname $(PORTSDIR)` ] ;\
	then \
		mkdir `dirname $(PORTSDIR)` ;\
	fi
	if [ ! -d $(PORTSDIR) ] ;\
	then \
		mkdir $(PORTSDIR) ;\
	fi

clean clobber: 
	for x in $(O_SHFILES) ;\
	do \
		rm -f $(SHDIR)/$$x ;\
	done
	@for i in $(DIRS) ;\
	do \
		echo "\tcd $$i && $(MAKE) -f *.mk $(MAKEARGS) $@" ;\
		if cd $$i ;\
		then \
			$(MAKE) -f *.mk $(MAKEARGS) $@ ;\
			cd .. ;\
		fi ;\
	done

size: all
	$(SIZE)
	@for i in $(DIRS) ;\
	do \
		echo "\tcd $$i && $(MAKE) -f *.mk $(MAKEARGS) $@" ;\
		if cd $$i ;\
		then \
			$(MAKE) -f *.mk $(MAKEARGS) $@ ;\
			cd .. ;\
		fi ;\
	done

strip: all
	$(STRIP)
	@for i in $(DIRS) ;\
	do \
		echo "\tcd $$i && $(MAKE) -f *.mk $(MAKEARGS) $@" ;\
		if cd $$i ;\
		then \
			$(MAKE) -f *.mk $(MAKEARGS) $@ ;\
			cd .. ;\
		fi ;\
	done

lintit:

