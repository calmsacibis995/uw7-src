#ident	"@(#)devintf:common/cmd/devintf/devices/attrs/list/list.mk	1.5.9.2"
#ident "$Header$"

include $(CMDRULES)

INSDIR		= $(USRSADM)/sysadm/menu/devices/devices/attrs/list
OWN=bin
GRP=bin

DESTDIR		= $(INSDIR)
HELPSRCDIR 	= .
SHFILES		=
FMTFILES	= maxcol
FMTFILE		= formatit
DISPFILES	= Form.list Text.list 
HELPFILES	= Help
SRCFILE		= maxcol.c

all: $(SHFILES) $(HELPFILES) $(FMTFILES) $(DISPFILES) $(FMTFILE)

$(FMTFILES): $(SRCFILE)
	$(CC) $(CFLAGS) $(DEFLIST) -o $(FMTFILES) $(SRCFILE) $(LDFLAGS) $(LDLIBS) $(SHLIBS)

$(FMTFILE):

$(HELPFILES):

$(DISPFILES):

# $(SHFILES):

clean:
	rm -f *.o maxcol

clobber: clean

size:

strip:

lintit:

install: $(DESTDIR) all
	for i in $(DISPFILES) ;\
	do \
		$(INS) -f $(DESTDIR) -m 0640 -u $(OWN) -g $(GRP) $$i ;\
	done
	for i in $(FMTFILES) ;\
	do \
		$(INS) -f $(DESTDIR) -m 0640 -u $(OWN) -g $(GRP) $$i ;\
	done
	for i in $(FMTFILE) ;\
	do \
		$(INS) -f $(DESTDIR) -m 0640 -u $(OWN) -g $(GRP) $$i ;\
	done
	for i in $(HELPFILES) ;\
	do \
		$(INS) -f $(DESTDIR) -m 0640 -u $(OWN) -g $(GRP) $(HELPSRCDIR)/$$i ;\
	done
#	for i in $(SHFILES) ;\
#	do \
#		$(INS) -m 750 -g bin -u bin -f $(DESTDIR) $$i ;\
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
