#!/sbin/sh
#ident	"@(#)drf:cmd/emergency_disk.sh	1.12.2.1"

# Clean up and then exit
cleanup()
{
	trap '' 1 2 15

	echo
	pfmt -l UX:drf -s error -g drf:1 "Emergency Recovery floppy creation aborted.\n\tDeleting the temporary files. Please wait.\n\n"
	[ -z "$PROTO" ] || {
		rm -rf ${PROTO}/${LCL_MACH}
		rm -rf ${PROTO}/stage
		$prt_CLN && rm -rf $PROTO
	}

	Gui_Exit 1
}

# Create the Disaster Recovery boot floppy
create_floppy()
{
	# script to set up, invoke prep_flop/cut_drf and clean up 

	MACH=drf_mach export MACH
	LCL_MACH=.${MACH} export LCL_MACH

	[ -z "$PROTO" ] && PROTO="/usr/work_$$"
	[ "$PROTO" = "/" ] && PROTO="/work_$$"
	export PROTO

	prt_CLN=false export prt_CLN
	[ -d $PROTO ] || {
		mkdir -p $PROTO
		prt_CLN=true
	}
	
	ROOT=$PROTO export ROOT

	df -b $PROTO | grep '^/' > /tmp/drf_$$_sz
	read a sp_avail < /tmp/drf_$$_sz
	SPC_need=${DRF_SPACE:-30720}
	[ $SPC_need -lt 28672 ] && SPC_need=30720
	[ $sp_avail -gt $SPC_need ] || {
	    echo
	    pfmt -l UX:drf -s error -g drf:2 " There is not enough space on the file system containing\n\t\tthe working directory %s.\n\n\t\tInvoke the command with different path name with -d option.\n\t\tThe file system containing this new path should have at least\n\t\t22 Mg of free space.\n\n" $PROTO
	    rm -f /tmp/drf_$$_sz
	    Gui_Exit 1
	}

	pfmt -l UX:drf -s nostd -g drf:8 "\n\tThis will take some time. Please wait...\n"

	/usr/lib/drf/prep_flop  
	err=$?

	rm -rf ${PROTO}/${LCL_MACH}
	rm -rf ${PROTO}/stage
	rm -f /tmp/drf_$$_sz
	$prt_CLN && rm -rf $PROTO
	return $err
}

Usage()
{
 
	echo
	pfmt -l UX:drf -s error -g drf:3 "Usage: emergency_disk [-d work_directory ] diskette1|diskette2.\n\n"
	Gui_Exit 1
}

Gui_Exit()
{
	$GuiInter && { 
		pfmt -l UX:drf -s NOSTD -g drf:29 "\tPress <ENTER> to return."
		read ans
	}
	exit $1
}

#
#main()

trap 'cleanup' 1 2 15

PROTO= export PROTO
GuiInter=false export GuiInter

while getopts 'gd:\?' c
do
	case $c in
		d)
			if [ -z "$OPTARG" ]
			then
				PROTO="/usr/work_$$"
			elif [ "$OPTARG" = "/" ]
			then
				PROTO="/work_$$"
			else
				PROTO="/$OPTARG/work_$$"
			fi
			;;
		g)
			GuiInter=true
			;;
		\?)
			Usage
			;;
		*)
			echo "\tInternal error during getopts.\n" 
			Gui_Exit 2
			;;
	esac
done
shift `expr $OPTIND - 1`

[ $# -eq 1 ] || Usage

MEDIUM=$1
export MEDIUM

echo $MEDIUM | grep -q diskette || Usage

devattr ${MEDIUM} 1>/dev/null 2>&1 ||
	{ echo
	  pfmt -l UX:drf -s error -g drf:4 "Device %s is not present in Device Database.\n" $MEDIUM; 
	  echo
	  Gui_Exit 1; }

FDRIVE_TMP=`devattr $MEDIUM fmtcmd|cut -f 3 -d " "|sed 's/t//'` export FDRIVE_TMP
FDRIVE=`basename $FDRIVE_TMP` export FDRIVE
BLKS=`devattr $MEDIUM capacity` export BLKS
BLKCYLS=`devattr $MEDIUM mkfscmd|cut -f 7 -d " "` export BLKCYLS


pfmt -l UX:drf -s NOSTD -g drf:28 "\n\tPlease insert a floppy and press <ENTER>."
read ans

teststr="The emergency_disk_test_string"
testfile=/tmp/tst_drf.$$

while echo "${teststr}" > ${testfile}
do
	if echo ${testfile} | cpio -oc -G STDIO -O /dev/dsk/${FDRIVE}t 2>/dev/null 1>&2
	then
		:	# The disk is formatted
	else
		echo
		pfmt -l UX:drf -s NOSTD -g drf:5 "	There is no floppy in the drive, or the floppy is\n	write-protected, or the floppy is not formatted.\n	Enter (f)ormat, (r)etry, or (q)uit: "
		read ans
		echo
		case "${ans:-q}" in

		  f) /usr/sbin/format -i1 /dev/rdsk/${FDRIVE}t > /dev/null 2>&1
		     continue
		     ;;

		  r) continue ;;
		
		  *) rm -f ${testfile}
			pfmt -l UX:drf -s NOSTD -g drf:30 "\n\tEmergency Recovery floppy creation aborted.\n\n"
		        Gui_Exit 0 ;;

		esac
	fi

	break
done

rm -f ${testfile}

DRF_LOG_FL=/tmp/drf_$$.log export DRF_LOG_FL

create_floppy 
ext_code=$?

if [ $ext_code -eq 0 ] 
then
	pfmt -l UX:drf -s NOSTD -g drf:6 "\n\tCreation of the Emergency Recovery boot floppy was successful.\n\n"
elif [ $ext_code -eq 2 ] 
then
	pfmt -l UX:drf -s NOSTD -g drf:30 "\n\tEmergency Recovery floppy creation aborted.\n\n"
	ext_code=0
else
	pfmt -l UX:drf -s NOSTD -g drf:7 "\n\tCreation of the Emergency Recovery boot floppy was NOT successful.\n\tCheck the %s log file for the reason for failure.\n\n" ${DRF_LOG_FL}
fi

Gui_Exit $ext_code  #Gui_Exit will exit

