#ident  "@(#)boot.mk	1.3"
#	copyright	"%c%"

include $(CMDRULES)

LOCALE	= it

BOOTSRC = ../installation/sysinst/locale/$(LOCALE)

BOOTFILES = menus/config.sh menus/help/UWdocs \
	menus/help/acl menus/help/acp menus/help/act_key \
	menus/help/alldocs menus/help/ask_serno menus/help/ast \
	menus/help/atm menus/help/audit menus/help/base menus/help/basex \
	menus/help/bkrs menus/help/bootcode menus/help/bsdcompat \
	menus/help/caninst menus/help/ccs menus/help/change_pkgs \
	menus/help/cmds menus/help/compaq menus/help/datetime \
	menus/help/dcu.select menus/help/deUWdocs \
	menus/help/dele menus/help/desktop \
	menus/help/destpart menus/help/destruct menus/help/dfs \
	menus/help/diskconf menus/help/diskopts menus/help/display \
	menus/help/dtTZ menus/help/dtclients menus/help/dtcontinent \
	menus/help/dtday menus/help/dthour menus/help/dtminute \
	menus/help/dtmonth menus/help/dttimezone menus/help/dtxt \
	menus/help/dtxtfonts menus/help/dtyear \
	menus/help/esUWdocs menus/help/esle \
	menus/help/filesys \
	menus/help/frUWdocs menus/help/frle \
	menus/help/fsadvopts menus/help/fsalttab menus/help/fsdisk \
	menus/help/fsdump menus/help/fshome menus/help/fshome2 \
	menus/help/fsroot menus/help/fssize menus/help/fsstand \
	menus/help/fsswap menus/help/fstmp menus/help/fstype menus/help/fsusr \
	menus/help/fsvar menus/help/fsvartmp menus/help/fsvolprivate \
	menus/help/genhelp menus/help/handbooks \
	menus/help/hba.select menus/help/help.main \
	menus/help/inet menus/help/initdisk menus/help/initkeyb \
	menus/help/initsysname menus/help/itUWdocs \
	menus/help/itle \
	menus/help/jaSDKdocs menus/help/jaUWdocs \
	menus/help/jale \
	menus/help/kbhelp menus/help/kbtype menus/help/lp menus/help/ls \
	menus/help/manpages menus/help/media.cdrom menus/help/media.disk \
	menus/help/media.ipx menus/help/media.tape menus/help/media.tcp \
	menus/help/menu_exit menus/help/mtfrun menus/help/mtfdev menus/help/na \
	menus/help/net.cable menus/help/net.dma menus/help/net.hw \
	menus/help/net.inter menus/help/net.ioadd menus/help/net.netmask \
	menus/help/net.ramadd menus/help/net.routeIP menus/help/net.serveIP \
	menus/help/net.server menus/help/net.slot menus/help/net.sysIP \
	menus/help/net.sysname menus/help/netmgt menus/help/netparams \
	menus/help/nfs menus/help/nics menus/help/nis menus/help/nsu \
	menus/help/nuc menus/help/nwnet menus/help/nwsup menus/help/oam \
	menus/help/osmp menus/help/partcyl menus/help/partdisk \
	menus/help/partpercent menus/help/partstatus menus/help/parttype \
	menus/help/platform menus/help/resetgeo \
	menus/help/restart menus/help/rpc \
	menus/help/scoadmin menus/help/scohelp \
	menus/help/serial menus/help/server menus/help/sid.select \
	menus/help/startinst \
	menus/help/surfanalys menus/help/sysname menus/help/terminf \
	menus/help/tricord menus/help/user_limit \
	menus/help/xclients menus/help/xcontrib menus/help/xdevsys \
	menus/help/xfonts menus/help/xserver menus/txtstrings

ICONVFILES = boot.fd boot.hd lang.footers lang.msgs smartmsg1

BOOTDIR = $(PROTO)/locale/$(LOCALE)

all:	$(ICONVFILES)

boot.fd:
	iconv -f 88591 -t PC437 $(BOOTSRC)/boot.fd >boot.fd

boot.hd:
	iconv -f 88591 -t PC437 $(BOOTSRC)/boot.hd >boot.hd

lang.footers:
	iconv -f 88591 -t PC437 $(BOOTSRC)/lang.footers >lang.footers

lang.msgs:
	iconv -f 88591 -t PC437 $(BOOTSRC)/lang.msgs >lang.msgs

smartmsg1:
	iconv -f 88591 -t PC437 $(BOOTSRC)/smartmsg1 >smartmsg1

install: $(BOOTSRC) $(ICONVFILES)
	[ -d $(BOOTDIR)/menus/help ] || mkdir -p $(BOOTDIR)/menus/help
	for i in $(BOOTFILES) ;\
	do \
		j=`dirname $$i` ;\
		$(INS) -f $(BOOTDIR)/$$j $(BOOTSRC)/$$i ;\
	done
	$(INS) -f $(BOOTDIR) boot.fd
	$(INS) -f $(BOOTDIR) boot.hd
	$(INS) -f $(BOOTDIR) lang.footers
	$(INS) -f $(BOOTDIR) lang.msgs
	$(INS) -f $(BOOTDIR) smartmsg1
	-rm -f $(BOOTDIR)/menus/help/help.mk
	ln -s $(PROTO)/locale/C/menus/help/help.mk $(BOOTDIR)/menus/help/help.mk

clean:

clobber:

