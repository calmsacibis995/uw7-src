#ident	"@(#)cfgintf:common/cmd/cfgintf/system/system.mk	1.11.6.2"
#ident "$Header$"

include $(CMDRULES)

OAMBASE		= $(USRSADM)/sysadm
BINDIR		= $(OAMBASE)/bin
DESTDIR		= $(OAMBASE)/menu/machinemgmt/configmgmt/system
HELPSRCDIR 	= .
SHFILES		=
FMTFILES	= 
DISPFILES	= Text.system
HELPFILES	= Help
OWN		= bin
GRP		= bin

all: $(SHFILES) $(HELPFILES) $(FMTFILES) $(DISPFILES) 

# $(FMTFILES):

$(HELPFILES):

$(DISPFILES):

clean:

clobber: clean

size:

strip:

lintit:

install: $(DESTDIR) all
	for i in $(DISPFILES) ;\
	do \
		$(INS) -m 640 -g $(GRP) -u $(OWN) -f $(DESTDIR) $$i ;\
	done
#	for i in $(FMTFILES) ;\
#		do \
#		$(INS) -m 640 -g $(GRP) -u $(OWN) -f $(DESTDIR) $$i ;\
#	done
	for i in $(HELPFILES) ;\
	do \
		$(INS) -m 640 -g $(GRP) -u $(OWN) -f $(DESTDIR) $(HELPSRCDIR)/$$i ;\
	done
#	for i in $(SHFILES) ;\
#	do \
#		$(INS) -m 750 -g $(GRP) -u $(OWN) -f $(DESTDIR) $$i ;\
#	done

$(DESTDIR):
	builddir() \
	{ \
		if [ ! -d $$1 ]; \
		then \
		    builddir `dirname $$1`; \
		    mkdir $$1; \
		fi \
	}; \
	builddir $(DESTDIR)
