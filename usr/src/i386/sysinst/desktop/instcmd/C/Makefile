#ident "@(#)Makefile	15.1	98/03/04"

include $(CMDRULES)

KB_SRC = $(ROOT)/$(MACH)/usr/lib/keyboard/C
BINDIR = $(PROTO)/desktop/instcmd
INSDIR = $(PROTO)/desktop/keyboards

KEYBOARDS = A01 AX US

KEYCOMP = $(BINDIR)/keycomp

all: $(KEYBOARDS)

$(KEYBOARDS): $(KB_SRC)/$$@ 
	@if [ -f keymap ] ;\
	then \
		rm -f keymap ;\
	fi
	@ln -s $(BINDIR)/keymap keymap
	$(KEYCOMP) $(KB_SRC)/$@ $@

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
clean:
	rm -f $(KEYBOARDS) keymap kbmap
	
clobber: clean
