#ident	"@(#)quick.mk	1.2"
#ident	"$Header$"

include $(CMDRULES)

OAMBASE=$(USRSADM)/sysadm
INSDIR = $(OAMBASE)/menu/ports/port_quick
BINDIR = $(OAMBASE)/bin

SHFILES = q-add q-rm

CFILES = isastream

O_DFILES = Form.add Form.rm Menu.ap Menu.rp Text.cfa Text.cfr Text.priv quick.menu Help

all: clean isastream shells

isastream: isastream.c

shells:
		cp q-add.sh q-add
		cp q-rm.sh q-rm

clean:
	-rm -f *.o $(SHFILES) $(CFILES)

clobber: clean
	-rm -f $(SHFILES) $(CFILES)

install: all $(INSDIR) $(TASKS)
	for i in $(O_DFILES) ;\
	do \
		$(INS) -m 644 -g $(GRP) -u $(OWN) -f $(INSDIR) $$i ;\
	done

	for i in $(SHFILES) ;\
	do \
		$(INS) -m 755 -g $(GRP) -u $(OWN) -f $(BINDIR) $$i ;\
	done

	for i in $(CFILES) ;\
	do \
		$(INS) -m 755 -g $(GRP) -u $(OWN) -f $(BINDIR) $$i ;\
	done

size: all

strip: all

lintit:


$(INSDIR):
	if [ ! -d $(INSDIR) ] ;\
	then \
		mkdir -p $(INSDIR) ;\
	fi
