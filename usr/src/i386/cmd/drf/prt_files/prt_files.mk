#ident	"@(#)drf:prt_files/prt_files.mk	1.6.1.1"

include $(CMDRULES)

PFILES  = helpwin staticlist step1rc step2rc

INSDIR    = $(USRLIB)/drf

all: $(PFILES) 

helpwin:
	@ln -s $(PROTO)/desktop/menus/$@ $@

step1rc:
	@ln -s $(PROTO)/desktop/scripts/$@ $@

step2rc:
	@ln -s $(PROTO)/desktop/scripts/$@ $@

staticlist:
	@ln -s $(PROTO)/desktop/$@ $@

funcrc:
	@ln -s $(PROTO)/desktop/scripts/$@ $@

install: all
	[ -d $(INSDIR) ] || mkdir -p $(INSDIR)
	@for i in $(PFILES) ;\
	do \
		$(INS) -f $(INSDIR) $$i ;\
	done

clean:
	rm -f $(PFILES) 

clobber: clean
