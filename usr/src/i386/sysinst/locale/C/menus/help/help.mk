#ident  "@(#)help.mk	15.1	98/03/04"

HELPFILES = \
	help.main.hcf\
	ask_serno.hcf\
	act_key.hcf\
	fsalttab.hcf\
	fsdump.hcf\
	fshome.hcf\
	fshome2.hcf\
	fsroot.hcf\
	fsstand.hcf\
	fsswap.hcf\
	fstmp.hcf\
	fsusr.hcf\
	fsvar.hcf\
	fsvartmp.hcf\
	fsvolprivate.hcf\
	inet.hcf\
	kbhelp.hcf\
	kbtype.hcf\
	kdb.hcf\
	net.cable.hcf\
	net.dma.hcf\
	net.hw.hcf\
	net.inter.hcf\
	net.ioadd.hcf\
	net.netmask.hcf\
	net.ramadd.hcf\
	net.routeIP.hcf\
	net.serveIP.hcf\
	net.server.hcf\
	net.slot.hcf\
	net.sysIP.hcf\
	net.sysname.hcf\
	netmgt.hcf\
	netparams.hcf\
	nfs.hcf\
	nics.hcf\
	nis.hcf\
	nsu.hcf\
	nuc.hcf\
	user_limit.hcf\
	cans.hcf\
	change_disk_ops.hcf\
	change_slices.hcf\
	check_media.hcf\
	check_preserve.hcf\
	date.hcf\
	dcu.hcf\
	disk_ops.hcf\
	disk_size.hcf\
	hba.hcf\
	keyboard.hcf\
	lang.hcf\
	license.hcf\
	media.hcf\
	name.hcf\
	owner.hcf\
	partition.hcf\
	password.hcf\
	pla.hcf\
	rusure.hcf\
	security.hcf\
	services.hcf\
	slices.hcf\
	welcome.hcf\
	whole_disk.hcf\
	tcpconf.hcf\
	ipxconf.hcf\
	nisconf.hcf\
	nics_detect.hcf\
	nics_select.hcf\
	nics_config.hcf\
	locale.hcf\
	zone.hcf \
	ad_flash0.hcf \
	ad_flash1.hcf \
	ad_flash2.hcf \
	ad_flash3.hcf \
	ad_flash4.hcf \
	ad_flash5.hcf 

LINKFILES = \
	fs8.hcf\
	fs6.hcf\
	fs4.hcf\
	fs12.hcf\
	fs1.hcf\
	fs10.hcf\
	fs2.hcf\
	fs13.hcf\
	fs3.hcf\
	fs11.hcf\
	fs16.hcf\
	fs15.hcf

all: clean $(HELPFILES) dolinks

$(HELPFILES): $(@:.hcf=)
	$(ROOT)/$(MACH)/usr/lib/winxksh/hcomp $(@:.hcf=)

dolinks:
	ln fsalttab.hcf fs8.hcf
	ln fsdump.hcf fs6.hcf
	ln fshome.hcf fs4.hcf
	ln fshome2.hcf fs12.hcf
	ln fsroot.hcf fs1.hcf
	ln fsstand.hcf fs10.hcf
	ln fsswap.hcf fs2.hcf
	ln fstmp.hcf fs13.hcf
	ln fsusr.hcf fs3.hcf
	ln fsvar.hcf fs11.hcf
	ln fsvartmp.hcf fs16.hcf
	ln fsvolprivate.hcf fs15.hcf

clean:
	rm -f $(HELPFILES) 
