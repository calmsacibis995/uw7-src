#ident	"@(#)net.mk	1.14"
#ident	"$Header$"

include $(UTSRULES)

MAKEFILE=	net.mk
KBASE = ..
LINTDIR = $(KBASE)/lintdir
DIR = net

NET = net.cf/Driver.o
LFILE = $(LINTDIR)/net.ln

FILES = netsubr.o
CFILES = netsubr.c
LFILES = netsubr.ln

SRCFILES = $(CFILES)

SUBDIRS = timod tirdwr loopback inet des sockmod rpc ktli lockmgr ntty \
	osocket isocket md5 nb ppp pf slip

all:	local FRC
	@for d in $(SUBDIRS); do \
		(cd $$d; echo "=== $(MAKE) -f $$d.mk all"; \
		 $(MAKE) -f $$d.mk all $(MAKEARGS)); \
	 done

local:	$(NET)

$(NET): $(FILES)
	$(LD) -r -o $(NET) $(FILES)

install: local FRC
	cd net.cf; $(IDINSTALL) -R$(CONF) -M net
	@for d in $(SUBDIRS); do \
		(cd $$d; echo "=== $(MAKE) -f $$d.mk install"; \
		 $(MAKE) -f $$d.mk install $(MAKEARGS)); \
	 done

clean:	localclean
	@for d in $(SUBDIRS); do \
		(cd $$d; echo "=== $(MAKE) -f $$d.mk clean"; \
		 $(MAKE) -f $$d.mk clean $(MAKEARGS)); \
	 done

localclean:
	-rm -f *.o $(LFILES) *.L $(NET)
	-rm -f $(VCOUT)

localclobber:	localclean
	-$(IDINSTALL) -R$(CONF) -d -e net


clobber:	localclobber
	@for d in $(SUBDIRS); do \
		(cd $$d; echo "=== $(MAKE) -f $$d.mk clobber"; \
		 $(MAKE) -f $$d.mk clobber $(MAKEARGS)); \
	 done

$(LINTDIR):
	-mkdir -p $@

lintit:	$(LFILE) FRC
	@for d in $(SUBDIRS); do \
		(cd $$d; echo "=== $(MAKE) -f $$d.mk lintit"; \
		 $(MAKE) -f $$d.mk lintit $(MAKEARGS)); \
	 done

$(LFILE):	$(LINTDIR) $(LFILES)
	-rm -f $(LFILE) `expr $(LFILE) : '\(.*\).ln'`.L
	for i in $(LFILES); do \
		cat $$i >> $(LFILE); \
		cat `basename $$i .ln`.L >> `expr $(LFILE) : '\(.*\).ln'`.L; \
	done

fnames:
	@for d in $(SUBDIRS); do \
	    (cd $$d; \
		$(MAKE) -f $$d.mk fnames $(MAKEARGS) | \
		$(SED) -e "s;^;$$d/;"); \
	done
	@for f in $(SRCFILES); do \
		echo $$f; \
	done

headinstall: localhead FRC
	@for d in $(SUBDIRS); do \
		(cd $$d; echo "=== $(MAKE) -f $$d.mk headinstall"; \
		 $(MAKE) -f $$d.mk headinstall $(MAKEARGS)); \
	 done

sysHeaders = \
	bitypes.h \
	convsa.h \
	dlpi.h \
	netconfig.h \
	netid.h \
	socket.h \
	socketvar.h \
	sockio.h \
	sockmod.h \
	tihdr.h \
	timod.h \
	tiuser.h \
	un.h \
	xti.h

netHeaders = \
	netsubr.h

localhead: $(sysHeaders)
	@-[ -d $(INC)/sys ] || mkdir -p $(INC)/sys
	@for f in $(sysHeaders); \
	 do \
	    $(INS) -f $(INC)/sys -m $(INCMODE) -u $(OWN) -g $(GRP) $$f; \
	 done

	@-[ -d $(INC)/net ] || mkdir -p $(INC)/net
	@for f in $(netHeaders); \
	 do \
	    $(INS) -f $(INC)/net -m $(INCMODE) -u $(OWN) -g $(GRP) $$f; \
	 done

FRC:

include $(UTSDEPEND)

depend::
	@for d in $(SUBDIRS); do \
		(cd $$d; echo "=== $(MAKE) -f $$d.mk depend";\
		 touch $$d.mk.dep;\
		 $(MAKE) -f $$d.mk depend $(MAKEARGS));\
	done

include $(MAKEFILE).dep

VCHDR = socket.vh sockio.vh xti.vh
VCOUT	= $(VCHDR:.vh=.h)

socket.h:	socket.vh
sockio.h:	sockio.vh
xti.h:		xti.vh
