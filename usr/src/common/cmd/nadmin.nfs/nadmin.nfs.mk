#ident	"@(#)nadmin.nfs.mk	1.2"
#ident "$Header$"
#makefile for nfs administration screens

include $(CMDRULES)

OAMBASE=$(USRSADM)/sysadm
TARGETDIR = $(OAMBASE)/menu/netservices/remote_files
MDIR = ../nadmin.nfs.mk

all:

install: all
	for i in * ; do \
		if [ -d $$i ] ; then \
			if [ ! -d $(TARGETDIR)/$$i ] ; then \
			mkdir -p $(TARGETDIR)/$$i  ;\
			fi ; \
			cd $$i ;\
			$(MAKE) install $(MAKEARGS) "TARGETDIR=$(TARGETDIR)/$$i" "MDIR=../$(MDIR)" -f $(MDIR);\
			cd .. ;\
		else \
			if [ $$i != "nadmin.nfs.mk" ] ;\
			then \
				echo "installing $$i" ;\
				$(INS) -m 644 -g bin -u bin -f $(TARGETDIR)  $$i ;\
			fi ;\
		fi  ;\
	done

clean:

clobber: clean

lintit:
