#ident	"@(#)terminfo:common/cmd/terminfo/OSR5/Makefile	1.1"

#include $(CMDRULES)

COMMON_PARTS=	../adds.ti \
		../annarbor.ti \
		../beehive.ti \
		../cdc.ti \
		../colorscan.ti \
		../contel.ti \
		../datamedia.ti \
		../diablo.ti \
		../dtterm.ti \
		../fortune.ti \
		../general.ti \
		../hardcopy.ti \
		../hazeltine.ti \
		../heath.ti \
		../homebrew.ti \
		../lsi.ti \
		../microterm.ti \
		../special.ti \
		../sperry.ti \
		../teleray.ti \
		../tymshare.ti \
		../visual.ti \
		../xterm.ti

PARTS=	header *.ti $(COMMON_PARTS) trailer

COMPDIR=./tmp

all:	terminfo.src

check:	all
	@[ -d $(COMPDIR) ] || mkdir -p $(COMPDIR)
	TERMINFO=$(COMPDIR) 2>&1 $(COMPILE) -v terminfo.src > errs
	@sh ./ckout

install cp: all
	@[ -d $(TERMDIR) ] || mkdir -p $(TERMDIR)
	@[ -d $(TERMDIR) ] || mkdir -p $(TERMDIR)
	cp terminfo.src $(TERMDIR)/terminfo.src.osr5
	sed '/^###/d' <euroinfo.src >$(TERMDIR)/euroinfo.src.osr5
	sed '/^###/d' <brasinfo.src >$(TERMDIR)/brasinfo.src.osr5

# Strip out internal SCO comments from the distributed version of terminfo.
terminfo.src:	$(PARTS)
	sed '/^###/d' $(PARTS) >terminfo.src

clobber clean:
	rm -f terminfo.src terminfo.cat errs nohup.out
	rm -rf $(COMPDIR)
