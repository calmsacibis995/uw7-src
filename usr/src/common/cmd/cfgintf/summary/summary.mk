#ident	"@(#)cfgintf:common/cmd/cfgintf/summary/summary.mk	1.11.8.2"
#ident "$Header$"

include $(CMDRULES)

OAMBASE		= $(USRSADM)/sysadm
BINDIR		= $(OAMBASE)/bin
DESTDIR		= $(OAMBASE)/menu/machinemgmt/configmgmt/summary
HELPSRCDIR 	= .
SHFILES		=
FMTFILES	= maxcol
DISPFILES	= Text.summary
HELPFILES	= Help
SRCFILE		= maxcol.c
OWN		= bin
GRP		= bin

all: $(SHFILES) $(HELPFILES) $(FMTFILES) $(DISPFILES) 

$(FMTFILES): $(SRCFILE)
	$(CC) $(CFLAGS) -o $(FMTFILES) $(SRCFILE) $(LDFLAGS) $(LDLIBS) \
		$(SHLIBS)

$(SRCFILE):

$(HELPFILES):

$(DISPFILES):

clean:
	rm -f *.o maxcol

clobber: clean

size:

strip:

lintit:

install: $(DESTDIR) all
	for i in $(DISPFILES) ;\
	do \
		$(INS) -m 640 -g $(GRP) -g $(OWN) -f $(DESTDIR) $$i ;\
	done
	for i in $(FMTFILES) ;\
		do \
		$(INS) -m 640 -g $(GRP) -g $(OWN) -f $(DESTDIR) $$i ;\
	done
	for i in $(HELPFILES) ;\
	do \
		$(INS) -m 640 -g $(GRP) -g $(OWN) -f $(DESTDIR) $(HELPSRCDIR)/$$i ;\
	done
#	for i in $(SHFILES) ;\
#	do \
#		$(INS) -m 750 -g $(GRP) -g $(OWN) -f $(DESTDIR) $$i ;\
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
