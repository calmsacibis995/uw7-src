#ident "@(#)Makefile	15.1	98/03/04"

include $(CMDRULES)

KB_SRC = $(ROOT)/$(MACH)/usr/lib/keyboard/8859-2
DK_SRC =  $(ROOT)/$(MACH)/usr/lib/mapchan
BINDIR = $(PROTO)/desktop/instcmd
INSDIR = $(PROTO)/desktop/keyboards

KEYBOARDS = CZ HU PL RO SI
DEAD_KEYS = 88592.dk

KEYCOMP = $(BINDIR)/keycomp
DKCOMP = $(BINDIR)/dkcomp

all: $(KEYBOARDS) $(DEAD_KEYS)

$(KEYBOARDS): $(KB_SRC)/$$@ 
	@if [ -f keymap ] ;\
	then \
		rm -f keymap ;\
	fi
	@ln -s $(BINDIR)/keymap keymap
	$(KEYCOMP) $(KB_SRC)/$@ $@

$(DEAD_KEYS): $(DK_SRC)/$$@
	$(DKCOMP) $(DK_SRC)/$@ $@

install: all
	@for i in $(KEYBOARDS) ;\
	do \
		if [ ! -d $(INSDIR)/$$i ] ;\
		then \
			mkdir -p $(INSDIR)/$$i ;\
		fi ;\
		cp $$i kbmap ;\
		$(INS) -f $(INSDIR)/$$i kbmap ;\
	done
	@rm kbmap
	cp $(DEAD_KEYS) dead_keys
	@for i in $(KEYBOARDS) ;\
	do \
		$(INS) -f $(INSDIR)/$$i dead_keys ;\
	done
	@rm dead_keys

clean:
	rm -f $(KEYBOARDS) $(DEAD_KEYS) keymap kbmap dead_keys
	
clobber: clean
