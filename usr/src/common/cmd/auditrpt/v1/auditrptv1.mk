#	copyright	"%c%"

#ident	"@(#)auditrptv1.mk	1.2"
#ident  "$Header$"

include $(CMDRULES)

INSDIR = $(ETC)/security/audit/auditrpt
OWN=root
GRP=audit
SRCDIR = .
LOCALDEF=-D_KMEMUSER
FRC = 
FLPLIBS = -lnsl
RPTLIBS = -lia

RPTSRCS  = adt_evtparse.c adt_mac.c adt_lvlin.c adt_lvlout.c adt_loadmap.c \
	  auditrptv1.c adt_optparse.c adt_getrec.c adt_print.c adt_proc.c
FLPSRCS  = auditfltrv1.c adt_getrec.c
RPTOBJS = $(RPTSRCS:.c=.o)
FLPOBJS = $(FLPSRCS:.c=.o)
MAINS = auditrptv1 auditfltrv1

all:	$(MAINS)

auditrptv1:	$(RPTOBJS)
	$(CC) $(RPTOBJS) -o $@ $(LDFLAGS) $(LDLIBS) $(RPTLIBS) $(SHLIBS)

auditfltrv1:	$(FLPOBJS)
	$(CC) $(FLPOBJS) -o $@ $(LDFLAGS) $(LDLIBS) $(FLPLIBS) $(SHLIBS)

adt_evtparse.o: adt_evtparse.c \
	$(INC)/string.h \
	$(INC)/sys/param.h \
	$(INC)/sys/vnode.h \
	$(INC)/sys/mac.h \
	$(INC)/sys/systm.h \
	$(INC)/sys/privilege.h \
	$(INC)/sys/proc.h \
	$(INC)/ctype.h \
	$(INC)/pfmt.h \
	$(INC)/sys/fcntl.h \
	$(INC)/sys/resource.h \
	audit.h \
	auditrec.h \
	auditrptv1.h \
	../auditrpt.h \
	$(INC)/limits.h

adt_getrec.o: adt_getrec.c \
	$(INC)/stdio.h \
	$(INC)/stdlib.h \
	$(INC)/string.h \
	$(INC)/sys/vnode.h \
	$(INC)/sys/param.h \
	$(INC)/sys/privilege.h \
	$(INC)/sys/proc.h \
	$(INC)/sys/systm.h \
	$(INC)/mac.h \
	$(INC)/pfmt.h \
	$(INC)/sys/fcntl.h \
	$(INC)/sys/resource.h \
	audit.h \
	auditrec.h \
	auditrptv1.h \
	../auditrpt.h \
	$(INC)/limits.h

adt_loadmap.o: adt_loadmap.c \
	$(INC)/stdlib.h \
	$(INC)/sys/param.h \
	$(INC)/sys/proc.h \
	$(INC)/sys/privilege.h \
	$(INC)/sys/systm.h \
	$(INC)/sys/vnode.h \
	$(INC)/mac.h \
	$(INC)/string.h \
	$(INC)/pfmt.h \
	$(INC)/sys/fcntl.h \
	$(INC)/sys/resource.h \
	audit.h \
	auditrec.h \
	auditrptv1.h \
	../auditrpt.h \
	$(INC)/limits.h

adt_lvlin.o: adt_lvlin.c \
	$(INC)/sys/types.h \
	$(INC)/sys/param.h \
	$(INC)/sys/time.h \
	$(INC)/mac.h \
	$(INC)/fcntl.h \
	$(INC)/string.h \
	$(INC)/errno.h \
	$(INC)/unistd.h

adt_lvlout.o: adt_lvlout.c \
	$(INC)/sys/types.h \
	$(INC)/sys/param.h \
	$(INC)/sys/time.h \
	$(INC)/mac.h \
	$(INC)/fcntl.h \
	$(INC)/stdio.h \
	$(INC)/string.h \
	$(INC)/malloc.h \
	$(INC)/errno.h \
	$(INC)/unistd.h

adt_mac.o: adt_mac.c \
	$(INC)/sys/types.h \
	$(INC)/unistd.h \
	$(INC)/sys/param.h \
	$(INC)/sys/proc.h \
	$(INC)/sys/vnode.h \
	$(INC)/fcntl.h \
	$(INC)/sys/systm.h \
	$(INC)/mac.h \
	audit.h

adt_optparse.o: adt_optparse.c \
	$(INC)/stdlib.h \
	$(INC)/ctype.h \
	$(INC)/time.h \
	$(INC)/string.h \
	$(INC)/sys/vnode.h \
	$(INC)/sys/privilege.h \
	$(INC)/sys/param.h \
	$(INC)/sys/proc.h \
	$(INC)/sys/systm.h \
	$(INC)/mac.h \
	$(INC)/sys/fcntl.h \
	$(INC)/sys/resource.h \
	audit.h \
	auditrec.h \
	$(INC)/pfmt.h \
	auditrptv1.h \
	../auditrpt.h \
	$(INC)/limits.h

adt_print.o: adt_print.c \
	$(INC)/stdio.h \
	$(INC)/errno.h \
	$(INC)/sys/vnode.h \
	$(INC)/acl.h \
	$(INC)/sys/param.h \
	$(INC)/time.h \
	$(INC)/sys/privilege.h \
	$(INC)/sys/proc.h \
	$(INC)/sys/systm.h \
	$(INC)/mac.h \
	$(INC)/sys/lock.h \
	$(INC)/sys/mount.h \
	$(INC)/pfmt.h \
	$(INC)/sys/file.h \
	$(INC)/sys/fcntl.h \
	$(INC)/string.h \
	$(INC)/stdlib.h \
	$(INC)/sys/resource.h \
	$(INC)/sys/mman.h \
	audit.h \
	auditrec.h \
	$(INC)/sys/covert.h \
	$(INC)/sys/mkdev.h \
	$(INC)/sys/mod.h \
	auditrptv1.h \
	../auditrpt.h \
	$(INC)/limits.h

adt_proc.o: adt_proc.c \
	$(INC)/stdlib.h \
	$(INC)/string.h \
	$(INC)/sys/vnode.h \
	$(INC)/sys/param.h \
	$(INC)/sys/privilege.h \
	$(INC)/sys/proc.h \
	$(INC)/sys/systm.h \
	$(INC)/mac.h \
	$(INC)/pfmt.h \
	$(INC)/sys/fcntl.h \
	$(INC)/sys/resource.h \
	audit.h \
	auditrec.h \
	auditrptv1.h \
	../auditrpt.h \
	$(INC)/limits.h

auditrptv1.o: auditrptv1.c \
	$(INC)/stdio.h \
	$(INC)/stdlib.h \
	$(INC)/sys/vnode.h \
	$(INC)/sys/param.h \
	$(INC)/string.h \
	$(INC)/sys/privilege.h \
	$(INC)/sys/proc.h \
	$(INC)/sys/systm.h \
	$(INC)/mac.h \
	$(INC)/sys/types.h \
	$(INC)/sys/stat.h \
	$(INC)/unistd.h \
	$(INC)/errno.h \
	$(INC)/locale.h \
	$(INC)/pfmt.h \
	$(INC)/sys/fcntl.h \
	$(INC)/sys/resource.h \
	audit.h \
	auditrec.h \
	auditrptv1.h \
	../auditrpt.h \
	$(INC)/limits.h

auditfltrv1.o: auditfltrv1.c \
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
	audit.h \
	auditrec.h \
	auditrptv1.h \
	../auditrpt.h \
	$(INC)/limits.h

clean:
	rm -f $(RPTOBJS) $(FLPOBJS)

clobber: clean
	rm -f $(MAINS)

install:	$(MAINS) $(INSDIR)
	$(INS) -f $(INSDIR) -m 0550 -u $(OWN) -g $(GRP) auditrptv1
	$(INS) -f $(INSDIR) -m 0550 -u $(OWN) -g $(GRP) auditfltrv1

strip:
	$(STRIP) $(MAINS)

lintit:
	$(LINT) $(LINTFLAGS) $(LOCALDEF) $(RPTSRCS)
	$(LINT) $(LINTFLAGS) $(LOCALDEF) $(FLPSRCS)

remove:
	cd $(INSDIR);	rm -f $(MAINS)

$(INSDIR):
	[ -d $@ ] || mkdir -p $@ ;\
		$(CH)chmod 755 $@ ;\
		$(CH)chown bin $@

partslist:
	@echo auditrptv1.mk $(SRCDIR) $(RPTSRCS) $(FLPSRCS) | tr ' ' '\012' | sort

product:
	@echo $(MAINS)  |  tr ' ' '\012'  | \
		sed -e 's;^;$(INSDIR)/;' -e 's;//*;/;g'

productdir:
	@echo $(INSDIR)
