
#ident	"@(#)auditrpt.mk	1.2"
#ident "$Header$"

#  /usr/src/common/cmd/auditrpt is the directory of all generic commands
#  whose executable reside in $(INSDIR).
#  Auditrpt specific commands are in subdirectories under auditrpt
#  named by version (ex: the generic auditrpt is in this directory and
#  built by this makefile, but the version 4 specific auditrpt is in
#  ./v4/*.c, built by ./v4/auditrptv4.mk)

include $(CMDRULES)

INSDIR = $(USRSBIN)
OWN=root
GRP=audit
SRCDIR = .
LOCALDEF=-D_KMEMUSER
FRC = 
FLPLIBS = -lgen -lnsl
RPTLIBS = -lia

RPTSRCS  = auditrpt.c
FLPSRCS  = auditfltr.c
RPTOBJS = $(RPTSRCS:.c=.o)
FLPOBJS = $(FLPSRCS:.c=.o)
MAINS = auditrpt auditfltr


#
# This is for the generic auditrpt command
#


auditrpt:	$(RPTOBJS)
	$(CC) $(RPTOBJS) -o $@ $(LDFLAGS) $(LDLIBS) $(RPTLIBS) $(SHLIBS)

auditfltr:	$(FLPOBJS)
	$(CC) $(FLPOBJS) -o $@ $(LDFLAGS) $(LDLIBS) $(FLPLIBS) $(SHLIBS)

auditrpt.o: auditrpt.c \
	$(INC)/stdio.h \
	$(INC)/stdlib.h \
	$(INC)/sys/param.h \
	$(INC)/string.h \
	$(INC)/mac.h \
	$(INC)/sys/types.h \
	$(INC)/sys/stat.h \
	$(INC)/unistd.h \
	$(INC)/errno.h \
	$(INC)/locale.h \
	$(INC)/pfmt.h \
	$(INC)/sys/resource.h \
	$(INC)/audit.h \
	$(INC)/sys/auditrec.h \
	auditrpt.h

#
# This is for the generic auditfltr command
#

auditfltr.o: auditfltr.c \
	$(INC)/rpc/types.h \
	$(INC)/rpc/xdr.h \
	$(INC)/stdio.h \
	$(INC)/stdlib.h \
	$(INC)/sys/param.h \
	$(INC)/sys/privilege.h \
	$(INC)/mac.h \
	$(INC)/fcntl.h \
	$(INC)/sys/vnode.h \
	$(INC)/string.h \
	$(INC)/unistd.h \
	$(INC)/locale.h \
	$(INC)/pfmt.h \
	$(INC)/sys/sysmacros.h \
	$(INC)/sys/resource.h \
	$(INC)/audit.h \
	$(INC)/sys/auditrec.h \
	auditrpt.h \
	$(INC)/limits.h

all:	$(MAINS)
#  This is to build all the auditrpt version specific commands
	@for i in *;\
	do\
	    if [ -d $$i -a -f $$i/auditrpt$$i.mk ]; \
		then \
		cd  $$i;\
		$(MAKE) -f auditrpt$$i.mk $(MAKEARGS) $@ ; \
		cd .. ; \
	    fi;\
	done

clean:
	rm -f $(RPTOBJS) $(FLPOBJS)
	@for i in *;\
	do\
	    if [ -d $$i -a -f $$i/auditrpt$$i.mk ]; \
		then \
		cd  $$i;\
		$(MAKE) -f auditrpt$$i.mk $(MAKEARGS) $@ ; \
		cd .. ; \
	    fi;\
	done

clobber: clean
	rm -f $(MAINS)
	@for i in *;\
	do\
	    if [ -d $$i -a -f $$i/auditrpt$$i.mk ]; \
		then \
		cd  $$i;\
		$(MAKE) -f auditrpt$$i.mk $(MAKEARGS) $@ ; \
		cd .. ; \
	    fi;\
	done

install:	$(MAINS) $(INSDIR)
	$(INS) -f $(INSDIR) -m 0550 -u $(OWN) -g $(GRP) auditrpt
	$(INS) -f $(INSDIR) -m 0550 -u $(OWN) -g $(GRP) auditfltr
	@for i in *;\
	do\
	    if [ -d $$i -a -f $$i/auditrpt$$i.mk ]; \
		then \
		cd  $$i;\
		$(MAKE) -f auditrpt$$i.mk $(MAKEARGS) $@ ; \
		cd .. ; \
	    fi;\
	done

strip:
	$(STRIP) $(MAINS)
	@for i in *;\
	do\
	    if [ -d $$i -a -f $$i/auditrpt$$i.mk ]; \
		then \
		cd  $$i;\
		$(MAKE) -f auditrpt$$i.mk $(MAKEARGS) $@ ; \
		cd .. ; \
	    fi;\
	done

lintit:
	$(LINT) $(LINTFLAGS) $(LOCALDEF) $(RPTSRCS)
	$(LINT) $(LINTFLAGS) $(LOCALDEF) $(FLPSRCS)
	@for i in *;\
	do\
	    if [ -d $$i -a -f $$i/auditrpt$$i.mk ]; \
		then \
		cd  $$i;\
		$(MAKE) -f auditrpt$$i.mk $(MAKEARGS) $@ ; \
		cd .. ; \
	    fi;\
	done

remove:
	cd $(INSDIR);	rm -f $(MAINS)
	@for i in *;\
	do\
	    if [ -d $$i -a -f $$i/auditrpt$$i.mk ]; \
		then \
		cd  $$i;\
		$(MAKE) -f auditrpt$$i.mk $(MAKEARGS) $@ ; \
		cd .. ; \
	    fi;\
	done

$(INSDIR):
	[ -d $@ ] || mkdir -p $@ ;\
		$(CH)chmod 755 $@ ;\
		$(CH)chown bin $@
	@for i in *;\
	do\
	    if [ -d $$i -a -f $$i/auditrpt$$i.mk ]; \
		then \
		cd  $$i;\
		$(MAKE) -f auditrpt$$i.mk $(MAKEARGS) $@ ; \
		cd .. ; \
	    fi;\
	done

partslist:
	@echo auditrpt.mk $(SRCDIR) $(RPTSRCS) $(FLPSRCS) | tr ' ' '\012' | sort
	@for i in *;\
	do\
	    if [ -d $$i -a -f $$i/auditrpt$$i.mk ]; \
		then \
		cd  $$i;\
		$(MAKE) -f auditrpt$$i.mk $(MAKEARGS) $@ ; \
		cd .. ; \
	    fi;\
	done

product:
	@echo $(MAINS)  |  tr ' ' '\012'  | \
		sed -e 's;^;$(INSDIR)/;' -e 's;//*;/;g'
	@for i in *;\
	do\
	    if [ -d $$i -a -f $$i/auditrpt$$i.mk ]; \
		then \
		cd  $$i;\
		$(MAKE) -f auditrpt$$i.mk $(MAKEARGS) $@ ; \
		cd .. ; \
	    fi;\
	done

productdir:
	@echo $(INSDIR)
	@for i in *;\
	do\
	    if [ -d $$i -a -f $$i/auditrpt$$i.mk ]; \
		then \
		cd  $$i;\
		$(MAKE) -f auditrpt$$i.mk $(MAKEARGS) $@ ; \
		cd .. ; \
	    fi;\
	done
