#ident "@(#)Makefile	15.1	98/03/04"

include $(CMDRULES)
BINDIR = $(PROTO)/desktop/instcmd
INSDIR = $(PROTO)/desktop/keyboards/code_sets

FCOMP = $(BINDIR)/fcomp -n
BMGR = $(BINDIR)/bmgr
FONTS = 88591 88592 88595 88597 88599
INPUT_FONTS = ISO8859-1 ISO8859-2 ISO8859-5 ISO8859-7 ISO8859-9 \
	gr.8x14 gr.8x16 

all: $(FONTS)

$(FONTS): 
	$(BMGR) gr.8x14
	$(BMGR) gr.8x16
	@mv gr.8x14.bm grbm.8x14
	@mv gr.8x16.bm grbm.8x16
	 $(FCOMP) ISO8859-1 88591
	 $(FCOMP) ISO8859-2 88592
	 $(FCOMP) ISO8859-5 88595
	 $(FCOMP) ISO8859-7 88597
	 $(FCOMP) ISO8859-9 88599

install:
	@if [ ! -d $(INSDIR) ] ;\
	then \
		mkdir -p $(INSDIR) ;\
	fi
	@for i in $(FONTS) ;\
	do \
		$(INS) -f $(INSDIR) $$i ;\
	done

clean:
	rm -f $(FONTS) grbm.8x14 grbm.8x16

clobber: clean
