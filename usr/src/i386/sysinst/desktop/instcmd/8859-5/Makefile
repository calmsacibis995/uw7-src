#ident "@(#)Makefile	15.1	98/03/04"

# Currently there are no dead key maps for this code set. Hence all
# sections related to dead key map compilation and installation
# have been commented out

include $(CMDRULES)

KB_SRC = $(ROOT)/$(MACH)/usr/lib/keyboard/8859-5
DK_SRC =  $(ROOT)/$(MACH)/usr/lib/mapchan
BINDIR = $(PROTO)/desktop/instcmd
INSDIR = $(PROTO)/desktop/keyboards

KEYBOARDS = RU
#DEAD_KEYS = 88595.dk

KEYCOMP = $(BINDIR)/keycomp
DKCOMP = $(BINDIR)/dkcomp

#all: $(KEYBOARDS) $(DEAD_KEYS)
all: $(KEYBOARDS)

$(KEYBOARDS): $(KB_SRC)/$$@ 
	@if [ -f keymap ] ;\
	then \
		rm -f keymap ;\
	fi
	@ln -s $(BINDIR)/keymap keymap
	$(KEYCOMP) $(KB_SRC)/$@ $@

#$(DEAD_KEYS): $(DK_SRC)/$$@
#	$(DKCOMP) $(DK_SRC)/$@ $@

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
#	cp $(DEAD_KEYS) dead_keys
#	@for i in $(KEYBOARDS) ;\
#	do \
#		$(INS) -f $(INSDIR)/$$i dead_keys ;\
#	done
#	@rm dead_keys

clean:
	rm -f $(KEYBOARDS) $(DEAD_KEYS) keymap kbmap dead_keys
	
clobber: clean
