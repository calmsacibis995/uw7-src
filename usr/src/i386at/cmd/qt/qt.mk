#ident	"@(#)qt:qt.mk	1.3.2.3"

include	$(CMDRULES)

TDIR	=  $(USRLIB)/tape
FDIRS	= \
	$(ROOT)/$(MACH)/usr/vmsys \
	$(ROOT)/$(MACH)/usr/vmsys/OBJECTS \
	$(ROOT)/$(MACH)/usr/vmsys/OBJECTS/tape
FACEDIR	= $(ROOT)/$(MACH)/usr/vmsys/OBJECTS/tape

all:

install:	all $(TDIR) $(FDIRS)
	$(INS) -f $(FACEDIR) -m 0755 -u bin -g bin Menu.tape
	$(INS) -f $(FACEDIR) -m 0755 -u bin -g bin Text.tape
	$(INS) -f $(FACEDIR) -m 0755 -u bin -g bin Text.fop
	$(INS) -f $(FACEDIR) -m 0755 -u bin -g bin Text.sop
	-@$(CP) admin.sh admin
	$(INS) -f $(FACEDIR) -m 0755 -u bin -g bin admin
	-@$(RM) -f admin

$(TDIR):
	mkdir $@
	$(CH)chmod 755 $@
	$(CH)chown root $@
	$(CH)chgrp bin $@

$(FDIRS):
	mkdir $@
	$(CH)chmod 755 $@
	$(CH)chown bin $@
	$(CH)chgrp bin $@

clean:
	rm -f *.o

clobber: clean
	-rm -f admin
