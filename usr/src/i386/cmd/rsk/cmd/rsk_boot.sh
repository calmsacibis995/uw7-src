#!/sbin/sh
#ident	"@(#)rsk_boot.sh	15.1"

# Clean up and then exit
cleanup()
{
	trap '' 1 2 15
	echo 
	pfmt -l UX:rsk -s error -g rsk:1 "Replicated System Kit boot floppy creation aborted.\n\n"

#If there's a /tmp/unixware.dat file around, the inet menu was interrupted
#restore the networking parameters before quitting
	[ -f /tmp/unixware.dat ] && {
		mv /etc/Onodename /etc/nodename
		mv /etc/inet/Ohosts /etc/inet/hosts
		setuname -n `cat /etc/nodename`
		SILENT_INSTALL=true ROOT=/ /etc/inet/menu
		rm /tmp/unixware.dat
	}
	[ -z "$PROTO" ] || {
		$KeepKernFiles || {
			pfmt -l UX:rsk -s error -g rsk:22 "\tDeleting the temporary files. Please wait.\n\n"
			[ -z "$LCL_MACH" ] || rm -rf ${PROTO}/${LCL_MACH}
			rm -rf ${PROTO}/stage
			$prt_CLN && rm -rf $PROTO
		}
	}

	Gui_Exit 1
}

# Create the Disaster Recovery boot floppy
create_floppy()
{
	# script to set up, invoke prep_flop/cut_rsk and clean up 

	MACH=rsk_mach export MACH
	LCL_MACH=.${MACH} export LCL_MACH

	[ -z "$PROTO" ] && PROTO="/usr/work_$$"
	[ "$PROTO" = "/" ] && PROTO="/work_$$"
	[ -d $PROTO ] || {
		mkdir -p $PROTO
		prt_CLN=true
	}
	export PROTO

	ROOT=$PROTO export ROOT

	df -b $PROTO | tail -1 > /tmp/rsk_$$_sz
	read a sp_avail < /tmp/rsk_$$_sz
	SPC_need=${DRF_SPACE:-22000}
	[ $SPC_need -lt 15000 ] && SPC_need=22000
	[ $SkipKernBuild ] && [ -f $PROTO/$LCL_MACH/stand/unix.tape ] && SPC_need=$sp_avail
	[ $sp_avail -lt $SPC_need ] && {
	    echo
	    pfmt -l UX:rsk -s error -g rsk:2 " There is not enough space on the file system containing\n\t\tthe working directory %s.\n\n\t\tInvoke the command with different path name with -d option.\n\t\tThe file system containing this new path should have at least\n\t\t22 MB of free space.\n\n" $PROTO
	    rm -f /tmp/rsk_$$_sz
	    Gui_Exit 1
	}

	pfmt -l UX:rsk -s nostd -g rsk:8 "\n\tThis will take some time. Please wait...\n" 2>&1

	/usr/lib/rsk/prep_flop  
	err=$?

	$KeepKernFiles || rm -rf ${PROTO}/${LCL_MACH}
	$KeepKernFiles || rm -rf ${PROTO}/stage
	rm -f /tmp/rsk_$$_sz
	$KeepKernFiles || {
		$prt_CLN && rm -rf $PROTO
	}
	return $err
}

Usage1()
{
 
	echo
	pfmt -l UX:rsk -s error -g rsk:3 "Usage: rsk_boot [-d work_directory] [-n node name] [-q] [-k] [-i] [-s serno act_key] diskette1.\n\n"
	Gui_Exit 1
}

Usage2()
{
 
	echo
	pfmt -l UX:rsk -s error -g rsk:9 "Usage: the '-i' option requires the use of the '-n node name' option.\n\n"
	Gui_Exit 1
}

Serno_valid()
{
 
	echo
	pfmt -l UX:rsk -s error -g rsk:10 "The serial number/activation key you supplied is invalid.\nPlease check that you have the correct strings\n\n"
	Gui_Exit 1
}

Gui_Exit()
{
	$GuiInter && { 
		pfmt -l UX:rsk -s NOSTD -g rsk:29 "\tPress <ENTER> to return." 2>&1
		read ans
	}
	exit $1
}

#
#main()

trap 'cleanup' 1 2 15

PROTO= export PROTO
NODE= export NODE
SERNO= export SERNO
INET=false export INET
GuiInter=false export GuiInter
SkipKernBuild=false export SkipKernBuild
KeepKernFiles=false export KeepKernFiles
prt_CLN=false export prt_CLN

while getopts 'gqkis:n:d:\?' c
do
	case $c in
		d)
			PROTO=$OPTARG
			;;
		n)
			NODE=$OPTARG
			;;
		g)
			GuiInter=true
			;;
		q)
			SkipKernBuild=true
			;;
		k)
			KeepKernFiles=true
			;;
		s)
			SERNO=$OPTARG
			;;
		i)
			INET=true
			;;
		\?)
			Usage1
			;;
		*)
			echo "\tInternal error during getopts.\n" 
			Gui_Exit 2
			;;
	esac
done
shift `expr $OPTIND - 1`

[ $# -eq 1 ] || Usage1

[ "$SERNO" = "" ] || /sbin/validate $SERNO || Serno_valid

$INET && {
[ -z "$NODE" ] || [ ! -d /etc/inet ]
} && Usage2

MEDIUM=$1

echo $MEDIUM | grep -q diskette || Usage1

devattr ${MEDIUM} 1>/dev/null 2>&1 ||
	{ echo
	  pfmt -l UX:rsk -s error -g rsk:4 "Device %s is not present in Device Database.\n" $MEDIUM; 
	  echo
	  Gui_Exit 1; }

FDRIVE_TMP=`devattr $MEDIUM fmtcmd|cut -f 3 -d " "|sed 's/t//'` export FDRIVE_TMP
FDRIVE=`basename $FDRIVE_TMP` export FDRIVE
BLKS=`devattr $MEDIUM capacity` export BLKS
BLKCYLS=`devattr $MEDIUM mkfscmd|cut -f 7 -d " "` export BLKCYLS


pfmt -l UX:rsk -s NOSTD -g rsk:28 "\n\tPlease insert a floppy and press <ENTER>." 2>&1
read ans

teststr="The emregency_disk_test_string"
testfile=/tmp/tst_rsk.$$

while echo "${teststr}" > ${testfile}
do
	if echo ${testfile} | cpio -oc -G STDIO -O /dev/dsk/${FDRIVE}t 2>/dev/null 1>&2
	then
		:	# The disk is formatted
	else
		echo
		pfmt -l UX:rsk -s NOSTD -g rsk:5 "	There is no floppy in the drive, or the floppy is\n	write-protected, or the floppy is not formatted.\n	Enter (f)ormat, (r)etry, or (q)uit: " 2>&1
		read ans
		echo
		case "${ans:-q}" in

		  f) /usr/sbin/format -i1 /dev/rdsk/${FDRIVE}t > /dev/null 2>&1
		     continue
		     ;;

		  r) continue ;;
		
		  *) rm -f ${testfile}
			pfmt -l UX:rsk -s NOSTD -g rsk:26 "\n\tReplicated System Kit boot floppy creation aborted.\n\n"
		        Gui_Exit 0 ;;

		esac
	fi

	break
done

rm -f ${testfile}

DRF_LOG_FL=/tmp/rsk_$$.log export DRF_LOG_FL

create_floppy 
ext_code=$?

if [ "$ext_code" = "0" ] 
then
	pfmt -l UX:rsk -s NOSTD -g rsk:6 "\nCreation of the Replicated System Kit boot floppy was successful.\n\n" 2>&1
elif [ "$ext_code" = "2" ] 
then
	pfmt -l UX:rsk -s NOSTD -g rsk:26 "\n\tReplicated System Kit boot floppy creation aborted.\n\n"
	ext_code=0
else
	pfmt -l UX:rsk -s NOSTD -g rsk:7 "\n\tCreation of the Replicated System Kit boot floppy was NOT successful.\n\tCheck the %s log file for the reason for failure.\n\n" ${DRF_LOG_FL}
fi

Gui_Exit $ext_code  #Gui_Exit will exit

