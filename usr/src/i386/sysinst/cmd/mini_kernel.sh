#!/usr/bin/ksh

#ident "@(#)mini_kernel.sh	15.1	98/03/04"


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
	$FAIL && exit 2
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
			print -u2 "$CMD: Fatal error -- cannot find kernel module $i."
			FAIL=true
		}
	done
	$FAIL && exit 2
	mv $* ..
	cd $ROOT/$MACH/etc/conf/sdevice.d
	for i in $(grep -l '[	 ]Y[	 ]' $*)
	do
		ed -s $i <<-END
			g/[	 ]Y[	 ]/s//	N	/
			w
			q
		END
	done
	cd $ROOT/$MACH/etc/conf/mdevice.d/.save
	mv $* ..
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
	$FAIL && exit 2
}

tune()
{
	idtune -f PAGES_UNLOCK 50
	idtune -f FLCKREC 100
	idtune -f NPROC 100
	idtune -f MAXUP 30
	idtune -f NHBUF 32
	idtune -f FD_DOOR_SENSE 0
	idtune -f ARG_MAX 20480 # Need large space for exported env variables
	idtune MEMFS_MAXKMEM 4096000
	idtune -f BDEV_RESERVE 255
	idtune -f CDEV_RESERVE 255
}

#main()

CMD=$0
PATH=$ROOT/$MACH/etc/conf/bin:$PATH
USAGE="Usage: $0 kdb|nokdb"

# December 10, 1996 -hah
# added at_toolkit through psm_time to STATIC_LIST which will allow
# BL6 prep.flop to build the mini_kernel.sh properly


STATIC_LIST="ansi asyc atup ca ccnv cdfs char cmux confmgr cram dcompat
dma eisa elf fdbuf fifofs fpe fs gentty gvid hpci iaf iasy intmap intp io
kd kdvm kernel kma ldterm mca mem memfs mm mod modksym name namefs nullzero pci
proc procfs pstart resmgr rtc sad sc01 sd01 sdi specfs st01 sum svc
sysclass ts udev util ws at_toolkit psm_i8254 psm_i8259 psm_mc146818 psm_time vtoc i2omsg i2otrans"

if [ "$LANG" = "ja" ]
then
	STATIC_LIST="$STATIC_LIST gsd fnt"
fi

DYNAMIC_LIST="fd s5 dosfs"

STUB_LIST="async audit coff dac event ipc log mac nfs prf pse rand rlogin segdev sysdump tpath xnamfs xque osocket segshm "

(( $# == 1 )) || {
	print -u2 $USAGE
	exit 2
}

case "$1" in
kdb)
	STATIC_LIST="$STATIC_LIST kdb kdb_util"
	print > $ROOT/$MACH/etc/conf/cf.d/kdb.rc
	;;
nokdb)
	DYNAMIC_LIST="$DYNAMIC_LIST kdb kdb_util"
	rm -f $ROOT/$MACH/etc/conf/cf.d/kdb.rc
	;;
nics)
	# obsolete; remove after caller goes away
	;;
*)
	print -u2 $USAGE
	exit 2
	;;
esac

turnoff
turnon $STATIC_LIST $DYNAMIC_LIST
stub $STUB_LIST
make_static $STATIC_LIST
tune
print $DYNAMIC_LIST
exit 0
