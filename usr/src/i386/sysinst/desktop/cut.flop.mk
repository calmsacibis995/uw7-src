#ident	"@(#)cut.flop.mk	15.1	98/03/04"


COMPRESS=bzip -9 -s64k
LCL_MACH=.$(MACH)
IDCONFUPDATE=$(ROOT)/$(LCL_MACH)/etc/conf/bin/$(PFX)idconfupdate 
#HBA_MODS = adsc cpqsc dpt ictha ide
HBA_MODS = 
COMPFILES = \
	stage3.blm platform.blm logo.img bootmsgs help.txt \
	unix resmgr memfs.meta memfs.fs
ALLFILES = $(COMPFILES) \
	dcmp.blm boot fdboot stage2.fdinst

all: $(ALLFILES)

stage3.blm: $(ROOT)/$(LCL_MACH)/etc/.boot/stage3.blm
	$(COMPRESS) $(ROOT)/$(LCL_MACH)/etc/.boot/stage3.blm >stage3.blm

platform.blm: $(ROOT)/$(LCL_MACH)/etc/.boot/platform.blm
	$(COMPRESS) $(ROOT)/$(LCL_MACH)/etc/.boot/platform.blm >platform.blm

bootmsgs: $(ROOT)/$(LCL_MACH)/etc/.boot/bootmsgs
	$(COMPRESS) $(ROOT)/$(LCL_MACH)/etc/.boot/bootmsgs >bootmsgs

help.txt: $(ROOT)/$(LCL_MACH)/etc/.boot/help.txt
	$(COMPRESS) $(ROOT)/$(LCL_MACH)/etc/.boot/help.txt >help.txt

logo.img: $(ROOT)/$(LCL_MACH)/etc/.boot/logo.img
	$(COMPRESS) $(ROOT)/$(LCL_MACH)/etc/.boot/logo.img >logo.img

unix: $(ROOT)/$(LCL_MACH)/stand/unix
	$(COMPRESS) $(ROOT)/$(LCL_MACH)/stand/unix > unix

memfs.meta: $(ROOT)/$(LCL_MACH)/stand/memfs.meta
	$(COMPRESS) $(ROOT)/$(LCL_MACH)/stand/memfs.meta > memfs.meta

memfs.fs: $(ROOT)/$(LCL_MACH)/stand/memfs.fs
	$(COMPRESS) $(ROOT)/$(LCL_MACH)/stand/memfs.fs > memfs.fs

dcmp.blm: $(ROOT)/$(LCL_MACH)/etc/.boot/dcmp.blm
	cp $(ROOT)/$(LCL_MACH)/etc/.boot/dcmp.blm dcmp.blm

boot: $(ROOT)/$(LCL_MACH)/etc/.boot/boot.fdinst
	cp $(ROOT)/$(LCL_MACH)/etc/.boot/boot.fdinst boot

fdboot: $(ROOT)/$(LCL_MACH)/etc/.boot/fdboot
	cp $(ROOT)/$(LCL_MACH)/etc/.boot/fdboot fdboot

stage2.fdinst: $(ROOT)/$(LCL_MACH)/etc/.boot/stage2.fdinst
	cp $(ROOT)/$(LCL_MACH)/etc/.boot/stage2.fdinst stage2.fdinst

resmgr: unix
	@cd $(ROOT)/$(LCL_MACH)/etc/conf/sdevice.d; \
	sed -e 's/[ 	]N[ 	]/	Y	/' .save/mfpd >mfpd; \
	cp ../mdevice.d/.save/mfpd ../mdevice.d/mfpd; \
	$(IDCONFUPDATE) -s -r $(ROOT)/$(LCL_MACH)/etc/conf -o $(ROOT)/$(LCL_MACH)/stand/resmgr
	@cd $(ROOT)/$(LCL_MACH)/etc/conf/sdevice.d; \
	rm -f mfpd ../mdevice.d/mfpd
	$(COMPRESS) $(ROOT)/$(LCL_MACH)/stand/resmgr > resmgr
