#ident	"@(#)devintf:common/cmd/devintf/groups/mbrship/mbrship.mk	1.8.8.2"
#ident "$Header$"

include $(CMDRULES)

INSDIR		= $(USRSADM)/sysadm/menu/devices/groups/mbrship
OWN=bin
GRP=bin

DESTDIR		= $(INSDIR)
HELPSRCDIR 	= .

SHFILES		=
FMTFILES	=
DISPFILES	= mbrship.menu
HELPFILES	= Help

SUBMAKES= add list remove

all: $(SHFILES) $(HELPFILES) $(FMTFILES) $(DISPFILES) 
	for submk in $(SUBMAKES) ; \
	do \
	    cd $$submk ; \
	    $(MAKE) -f $$submk.mk $(MAKEARGS) $@ ; \
	    cd .. ; \
	done

# $(SHFILES):

$(HELPFILES):

# $(FMTFILES):

$(DISPFILES):

clean clobber strip size lintit:
	for submk in $(SUBMAKES) ; \
	do \
	    cd $$submk ; \
	    $(MAKE) -f $$submk.mk $(MAKEARGS) $@ ; \
	    cd .. ; \
	done

install: $(DESTDIR) all
	for submk in $(SUBMAKES) ; \
	do \
	    cd $$submk ; \
	    $(MAKE) -f $$submk.mk $(MAKEARGS) $@ ; \
	    cd .. ; \
	done
#	for i in $(SHFILES) ;\
#	do \
#		$(INS) -m 640 -g bin -u bin -f $(DESTDIR) $$i ;\
#	done
	for i in $(HELPFILES) ;\
	do \
		$(INS) -f $(DESTDIR) -m 0640 -u $(OWN) -g $(GRP) $(HELPSRCDIR)/$$i ;\
	done
#	for i in $(FMTFILES) ;\
#	do \
#		$(INS) -m 640 -g bin -u bin -f $(DESTDIR) $$i ;\
#	done
	for i in $(DISPFILES) ;\
	do \
		$(INS) -f $(DESTDIR) -m 0640 -u $(OWN) -g $(GRP) $$i ;\
	done

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
