#--------------------------------------------------------------------------
#	@(#)psrsetupOsa.mk	1.2
#       Copyright (C) The Santa Cruz Operation, 1997.
#       This Module contains Proprietary Information of
#       The Santa Cruz Operation, and should be treated as Confidential.
#--------------------------------------------------------------------------

TOP=..
DOSMROOT=../..

include $(DOSMROOT)/make.inc.dosm
include $(TOP)/make.inc

DESTDIR=$(INSDIR)/usr/lib/scoadmin/psrsetup

FLIST=psrsetupOsa.msg.tcl psrsetupOsa.cdt psrsetupOsa.procs

ALL=psrsetupOsa


all: $(ALL)

install:	all nls
	@-[ -d $(DESTDIR) ] || mkdir -p $(DESTDIR)
	$(INS) -f $(DESTDIR) -m 755 -u $(OWN) -g $(GRP) psrsetupOsa

psrsetupOsa:	$(FLIST)
	$(SCRIPTSTRIP) -b -o $@ $(FLIST)

psrsetupOsa.msg.tcl:	NLS/$(MSGDEFAULT)/psrsetupOsa.msg
	$(MKCATDECL) -c -i NLS/psrsetupOsa.mod -m SCO_PSRSETUPOSA NLS/$(MSGDEFAULT)/psrsetupOsa.msg


nls:
	$(DONLS) -r $(DOSMROOT) -p psrsetupOsa -d psrsetupOsa -s NLS install

clean:
	$(DONLS) -r $(DOSMROOT) -p psrsetupOsa -d psrsetupOsa -s NLS $@

clobber:	clean
	$(DONLS) -r $(DOSMROOT) -p psrsetupOsa -d psrsetupOsa -s NLS $@
	-rm -f psrsetupOsa psrsetupOsa.msg.tcl


