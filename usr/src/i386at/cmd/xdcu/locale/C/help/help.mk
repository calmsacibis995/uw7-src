#ident	"@(#)dcu:locale/C/help/help.mk	1.5"
HELPFILES = \
dcu.addboard.hcf \
dcu.all.hcf \
dcu.apply.hcf \
dcu.board.hcf \
dcu.brdsum.hcf \
dcu.bsp.hcf \
dcu.cancel.hcf \
dcu.com.hcf \
dcu.driver.hcf \
dcu.drivsum.hcf \
dcu.main.hcf \
dcu.misc.hcf \
dcu.network.hcf \
dcu.restart.hcf \
dcu.return.hcf \
dcu.scsi.hcf \
dcu.sound.hcf \
dcu.video.hcf \
dcu.whatis.hcf \
kbhelp.hcf

all: $(HELPFILES)

$(HELPFILES): $(@:.hcf=)
	$(ROOT)/$(MACH)/usr/lib/winxksh/hcomp $(@:.hcf=)
