#!/usr/bin/ksh

#ident	"@(#)drf:cmd/mini_kernel.sh	1.1"

turnoff()
{
	cd $ROOT/$MACH/etc/conf/sdevice.d
	[ -d .save ] || mkdir .save
	mv * .save
	cd $ROOT/$MACH/etc/conf/mdevice.d
	[ -d .save ] || mkdir .save
	mv * .save
}

turnon()
{
	FAIL=false
	cd $ROOT/$MACH/etc/conf/sdevice.d/.save
	for i
	do
		[ -f $i ] || {
			print -u2 "$CMD: Fatal error -- cannot find kernel module $i."
			FAIL=true
		}
	done
	$FAIL && exit 1
	mv $* ..
	cd $ROOT/$MACH/etc/conf/sdevice.d
	for i in $(grep -l '[	 ]N[	 ]' $*)
	do
		# Some System files (like asyc's) have both Y and N entries.
		# Do not edit such files.
		case "$(print $(<$i))" in
		*' Y '*)
			continue
			;;
		esac
		ed -s $i <<-END
			g/[	 ]N[	 ]/s//	Y	/
			w
			q
		END
	done
	cd $ROOT/$MACH/etc/conf/mdevice.d/.save
	mv $* ..
}

stub()
{
	FAIL=false
	cd $ROOT/$MACH/etc/conf/sdevice.d/.save
	for i
	do
		[ -f $i ] || {
			if [ ! -f $ROOT/$MACH/etc/conf/mdevice.d/.save/$i ]
			then
				print -u2 "$CMD: Fatal error -- cannot find kernel module $i."
				FAIL=true
			fi
		}
	done
	$FAIL && exit 1
	mv $* .. 2>/dev/null
	cd $ROOT/$MACH/etc/conf/sdevice.d
	for i in $(grep -l '[	 ]Y[	 ]' $* 2>/dev/null)
	do
		ed -s $i <<-END
			g/[	 ]Y[	 ]/s//	N	/
			w
			q
		END
	done
	cd $ROOT/$MACH/etc/conf/mdevice.d/.save
	mv $* .. 2>/dev/null
}

make_static()
{
	FAIL=false
	cd $ROOT/$MACH/etc/conf/sdevice.d
	for i
	do
		case "$(print $(<$i))" in
		*'$version 2 $static'*)
			continue
			;;
		*'$version 2'*)
			;;
		*)
			print -u2 "$CMD: Fatal error -- kernel module $i is not version 2."
			FAIL=true
			continue
			;;
		esac
		ed -s $i <<-END
			/\$version/a
			\$static
			.
			w
			q
		END
	done
	$FAIL && exit 1
}

tune()
{
	/etc/conf/bin/idtune -f PAGES_UNLOCK 50
	/etc/conf/bin/idtune -f FLCKREC 100
	/etc/conf/bin/idtune -f NPROC 100
	/etc/conf/bin/idtune -f MAXUP 30
	/etc/conf/bin/idtune -f NHBUF 32
	/etc/conf/bin/idtune -f FD_DOOR_SENSE 0
	/etc/conf/bin/idtune -f ARG_MAX 20480 # Need large space for exported env variables
	/etc/conf/bin/idtune -f MEMFS_MAXKMEM 4096000
	/etc/conf/bin/idtune -f BDEV_RESERVE 255
	/etc/conf/bin/idtune -f CDEV_RESERVE 255
}

make_space()
{
	cd $ROOT/$MACH/etc/conf/sdevice.d/.save
	if [ "`echo *`" != "*" ]
	then
		for i in *
		do
			rm -rf $ROOT/$MACH/etc/conf/pack.d/$i
			rm -rf $ROOT/$MACH/etc/conf/mod.d/$i
			rm -rf $ROOT/$MACH/etc/conf/mtune.d/$i
			rm -rf $ROOT/$MACH/etc/conf/autotune.d/$i
		done
	fi
	cd ..
	rm -rf .save
	cd $ROOT/$MACH/etc/conf/mdevice.d/.save
	if [ "`echo *`" != "*" ]
	then
		for i in *
		do
			rm -rf $ROOT/$MACH/etc/conf/pack.d/$i
			rm -rf $ROOT/$MACH/etc/conf/mod.d/$i
			rm -rf $ROOT/$MACH/etc/conf/mtune.d/$i
			rm -rf $ROOT/$MACH/etc/conf/autotune.d/$i
		done
	fi
	cd ..
	rm -rf .save
	rm -rf $ROOT/$MACH/etc/conf/cf.d/stune*
}

#main()

CMD=$0
USAGE="Usage: $0 kdb|nokdb"

# December 10, 1996 -hah
# added at_toolkit through psm_time to STATIC_LIST which will allow
# BL6 prep.flop to build the mini_kernel.sh properly


STATIC_LIST="ansi asyc atup ca ccnv cdfs char cmux confmgr cram dcompat
dma eisa elf fd fdbuf fifofs fpe fs gentty gvid hpci iaf iasy intmap intp io
kd kdvm kernel kma ldterm mca mem memfs mm mod modksym mtrr name namefs nullzero
pci proc procfs pstart resmgr rtc sad sc01 sd01 sdi specfs st01 sum svc
sysclass ts udev util ws at_toolkit psm_i8254 psm_i8259 psm_mc146818
psm_time vtoc i2omsg i2otrans"

# Add any mods which may not exist on a minimal install, but do on the machine
if [ -f /etc/conf/sdevice.d/mpio ]
then
	STATIC_LIST="$STATIC_LIST mpio"
fi

if [ "$LANGS" = "ja" ]
then
	STATIC_LIST="$STATIC_LIST gsd fnt"
fi

DYNAMIC_LIST="$MODLIST"

STUB_LIST="async audit coff dac event ipc log mac nfs prf pse rand rlogin segdev sysdump tpath xnamfs xque osocket segshm"

# Add any stubs which may not exist on a minimal install, but do on the machine
if [ -f /etc/conf/sdevice.d/clariion ]
then
	STUB_LIST="$STUB_LIST clariion"
fi
if [ -f /etc/conf/sdevice.d/merge ]
then
	STUB_LIST="$STUB_LIST merge"
fi

case "$1" in
kdb)
	if [ -f /etc/conf/sdevice.d/kdb -a -f /etc/conf/sdevice.d/kdb_util ]
	then
		STATIC_LIST="$STATIC_LIST kdb kdb_util"
		print > $ROOT/$MACH/etc/conf/cf.d/kdb.rc
	else
		STUB_LIST="$STUB_LIST kdb_util"
		rm -f $ROOT/$MACH/etc/conf/cf.d/kdb.rc
	fi
	;;
nokdb)
	if [ -f /etc/conf/sdevice.d/kdb -a -f /etc/conf/sdevice.d/kdb_util ]
	then
		DYNAMIC_LIST="$DYNAMIC_LIST kdb kdb_util"
		rm -f $ROOT/$MACH/etc/conf/cf.d/kdb.rc
	else
		STUB_LIST="$STUB_LIST kdb_util"
		rm -f $ROOT/$MACH/etc/conf/cf.d/kdb.rc
	fi
	;;
nics)
	# obsolete; remove after caller goes away
	;;
*)
	print -u2 $USAGE
	exit 1
	;;
esac

turnoff
turnon $STATIC_LIST $DYNAMIC_LIST
stub $STUB_LIST
make_static $STATIC_LIST
tune
make_space
print $DYNAMIC_LIST
exit 0
