#!/sbin/sh
#	copyright	"%c%"

#ident	"@(#)xcprestore:i386at/xcpcmd/restore/restore.sh	1.1.2.4"
#ident  "$Header$"

exit_msg () {
	[ "$1" ] && echo $1 >&2
	echo $USAGE >&2
	exit 1
}
Get_Tape_Device_Node() {
	NUM_TP_DRIVES=$1
	grep qtape /tmp/Xdev$$ >/tmp/Xtape$$	#seperate tape entries 

	# set default tape device node to ctape1 or the first tape node
	# in $DEVTAPE
	case $NUM_TP_DRIVES in
		1)	cp /tmp/Xtape$$ /tmp/Xtmp$$
			read N D T </tmp/Xtape$$
			dirname $D >/tmp/Xtpdir$$
			;;
		*)	while read N D T
			do
				[ ! "$DEV" -o "$N" = ctape1 ] || {
					DEV=$D 
					DEVNAME=$N
					echo $N $D $T >/tmp/Xtmp$$
					dirname $D >/tmp/Xtpdir$$
					[ "$N" = ctape1 ] && break
				}
			done </tmp/Xtape$$	;;
	esac
	
	read DEFAULT_TP_NAME DEFAULT_TP_DEV TYPE </tmp/Xtmp$$

	# make a list of installed tape nodes
	for DIR in `cat /tmp/Xtpdir$$`
	do
		ls $DIR/* >>/tmp/Xtp_ls$$
	done
	sort /tmp/Xtp_ls$$ | uniq >/tmp/Xtp_ls1$$

	# prepend numbers to the list of tape nodes to present to the user.
	i=1
	while read D
	do
		SP=" "		#space
		[ $i -ge 10 ] &&   unset SP
		echo "$SP$i.  $D" >>/tmp/Xtpmenu$$
		i=`expr $i + 1 `
	done </tmp/Xtp_ls1$$
	TOT=`expr $i - 1`
	
	MSG0="Select menu number for the backup tape from the following nodes:"
	MSG1=`cat /tmp/Xtpmenu$$`
	MSG2="Hit return to use $DEFAULT_TP_DEV ($DEFAULT_TP_NAME). -->\c"
	# Get user selection.
	while true
	do
		echo "$MSG0\n$MSG1\n$MSG2"
		read DEV </dev/tty
		[ ! "$DEV"  ] &&  {
			grep "\.  $DEFAULT_TP_DEV" /tmp/Xtpmenu$$ >/tmp/Xtpno$$ 
			ln /tmp/Xtmp$$ /tmp/Xtp_use$$  # user selected default
			break
		}
		grep "^ *$DEV\.  " /tmp/Xtpmenu$$ >/tmp/Xtpno$$ && break
		echo "$DEV is not a menu number"
	done
	read NUM DEV </tmp/Xtpno$$
	[ -s /tmp/Xtp_use$$ ] && return

	# determine the device name in /etc/device.tab for the selected node
	# and save NAME NODE TYPE in /tmp/Xtp_use$$

	D_MAJ=`ls -l $DEV | sed 's/  */;/g' \
			  | cut -d";" -f5 | cut -d"," -f1`
	while read N D T
	do
		MAJ=`ls -l $D | sed 's/  */;/g' \
			      | cut -d";" -f5 | cut -d"," -f1`
		[ $MAJ -eq $D_MAJ ] && {
			echo $N $DEV $T >/tmp/Xtp_use$$
			break
		}
	done </tmp/Xtape$$
}

clean_up() {
	rm -f /tmp/X*$$ /tmp/R.*$$ /tmp/cplst$$ /tmp/flp_index
	[ ${ULIMIT} -ne 0 ] && ulimit ${ULIMIT}
}

# main()

# PURPOSE: Restore ${CPIO} files from floppy disk or cartridge tape
# ---------------------------------------------------------------------
#	Options:
#	c - complete restore
#	d - device; defaults to /dev/rdsk/f0, if t option is not used
#	t - tape device being used
#	o - overwrite files
#	i - index file

ULIMIT=0
USAGE="Usage:\t$0 [ -d <device> ] [ -c ] [ -i ] [ -o ] [ -t ] [pattern [pattern] ...]"
MEDIA="floppy disk"
TAPE="cartridge tape"
CPIO=cpio
CPIOFLGS="cik"
BLKSIZE="-B"
DEV=""
PATTERNS=
SFLAG=
overwrite=false

# clean_up before termination.
trap "clean_up;\
      echo \"You have cancelled the restore.\"; trap 0; exit " 1 2 3 9 15

# clean-up after a normal exit.
trap " clean_up; trap 0; exit " 0

type=c
while getopts d:itcos c
do
	case $c in
	c)	type=c
		;;
	d)	DEV="$OPTARG"
		;;
	t)	MEDIA="$TAPE"
		TFLAG=YES
		;;
	o)	CPIOFLGS="u$CPIOFLGS"
		overwrite=true
		;;
	i)	type=i
		;;
	s)	SFLAG=s
		;;
	\?)	echo "$USAGE" >&2
		exit 0
		;;
	*)	exit_msg ;;
	esac
done
shift `expr $OPTIND - 1`

while [ -n "$1" ]
do
	PATTERNS="\"$1\" $PATTERNS"
	shift
done

# $DEV, if set, must be a  readable file
[ "$DEV" -a  ! -r "$DEV" ] && {
	exit_msg "$0: $DEV - no such device"
}

# get the removable media entries in /etc/device.tab into the file /tmp/Xdev$$
# For these entries, device type can be tape, diskette or mden(also 
# used for floppy diskette).

DEVTAB=/etc/device.tab
getdev -a "removable=true" >/tmp/Xdevnm$$
while read DEVNAME 
do
	DEVICE=`grep "^$DEVNAME:" $DEVTAB | cut -d":" -f2`
	DEVTYPE=`devattr $DEVNAME  type`
	echo $DEVNAME $DEVICE $DEVTYPE >>/tmp/Xdev$$
done </tmp/Xdevnm$$

#
# If user is root, then raise the ulimit
#

id | grep "uid=0" > /dev/null
if [ $? -eq 0 ]
then
	ULIMIT=1
	ulimit 1048571
fi

# If device node is specified on the command line, determine if it is
# floppy disk, or cartridge tape or neither of the two. If the device node is
# a character/block special file match its major number with the major numbers
# of the removable entries in /etc/device.tab. Matching major number will 
# help set the device type as well as DEVNAME.
# reset MEDIA if TYPE=qtape and TFLAG is not set.
# If device specified is a floppy disk, then set FLOP_DRIVE to 0 or 1 depending
# upon whether DEV_MINOR is even or odd. This makes the prompt for medium to 
# to be very specific.

[ "$DEV" ] && {
	[ -c "$DEV" -o -b "$DEV" ] && {
		MAJ_MIN=`ls -l $DEV | sed 's/  */;/g' | cut -d";" -f5`
		DEV_MAJOR=`echo $MAJ_MIN | cut -d"," -f1`
	}
	[ "$DEV_MAJOR" ] && {
		while read NAME DEVICE DEVTYPE
		do
			MAJ=`ls -l $DEVICE | sed 's/  */;/g' | \
					     cut -d";" -f5 | cut -d"," -f1`
			[ $MAJ -eq $DEV_MAJOR ] && {
				TYPE=$DEVTYPE
				DEVNAME=$NAME
				echo $NAME $DEVICE $DEVTYPE >/tmp/Xdev_1_$$
				break
			}
		done </tmp/Xdev$$
	}
	[ -s /tmp/Xdev_1_$$ ] &&
		read DEVNAME DEVICE TYPE </tmp/Xdev_1_$$
	case "$TYPE" in
	    qtape)  MEDIA="$TAPE"	
		    [ "$TFLAG" ] || 
		       echo "$0: -t option not used for device $DEV" >&2 ;;

	    mden|diskette)  DEV_MINOR=`echo $MAJ_MIN | cut -d"," -f2 `
			    FLOP_DRIVE=`expr $DEV_MINOR % 2`
			    INPUT_DRIVE="floppy drive $FLOP_DRIVE" ;;

	    "")	    exit_msg "$0: $DEV is not a removable medium" ;;
	     *)	    exit_msg "$0: device type for $DEV is unknown" ;;
	esac

	[ "$TFLAG" -a "$MEDIA" != "$TAPE" ] &&
		exit_msg "$0: -t option can not be used for the device $DEV"

}

# if DEV is not set with the t option, ask the user to make a 
# selection from a menu of tape nodes installed on the system.
[ ! "$DEV"  -a  "$MEDIA" = "$TAPE" ] && {

	NUM_TP_DRIVES=`grep qtape /tmp/Xdev$$ | wc -l `
	[ $NUM_TP_DRIVES -eq 0 ] && 
		exit_msg "$0: No tape entry in the database /etc/device.tab"

	Get_Tape_Device_Node $NUM_TP_DRIVES
	read DEVNAME DEV JUNK </tmp/Xtp_use$$
}

# if DEV is not set, ask the user to make a selection  of the floppy drive,
# if there are more than one floppy drive.
[ ! "$DEV"  -a  "$MEDIA" = "floppy disk" ] && {
	DEV="/dev/rdsk/f0"
	FLOP_DRIVE=0
	/sbin/flop_num
	[ $? -gt 1 ] && {
		while true
		do
			echo "\nThe system has two floppy drives."
			echo "Strike ENTER to backup to drive 0"
			echo "or 1 to backup to drive 1.  \c"
			read ans
			case "$ans" in
				0 | "")	break ;;
				1 )	DEV="/dev/rdsk/f1"
					FLOP_DRIVE=1
					break ;;
				* )	continue ;;
			esac
		done
	}
	INPUT_DRIVE="floppy drive $FLOP_DRIVE"
}

#  For tape, reset BLKSIZE.
[ "$MEDIA" = "$TAPE" ] && {
	BLKSIZE=" -C 102400 "
	INPUT_DRIVE="the tape drive"
}
CPIOFLGS="$CPIOFLGS $BLKSIZE"

MESSAGE="You may remove the archive from $INPUT_DRIVE. To exit, strike 'q' followed
by ENTER.  To continue, insert the next archive and strike the ENTER key. "

case $type in
c )
	if [ "$SFLAG" = s ]
	then
		message -u "Be sure the first archive volume is inserted." >&2
	else
		message -u "Insert the backup archive in $INPUT_DRIVE." >&2
	fi
	echo "            Restore in progress\n\n" >&2

	trap '' 1 2 3 9 15
	eval $CPIO -dmv${CPIOFLGS} -M \"'$MESSAGE'\" -I "$DEV" $PATTERNS 2>/tmp/R.err$$ | tee /tmp/R.chk$$ 
	err=$?
	grep "ERROR.* open" /tmp/R.err$$ && exit 1
	trap 1 2 3 9 15
	if [ $err -eq 4 ]
	then
		echo "You have cancelled the restore" >&2
		exit 1

	elif [ $err -ne 0 ]
	then
		if [ "$overwrite" != "true" ]
		then
			# overwrite was not chosen so errors may be of the form
			# cpio: Existing "file" same age or newer
			# grep these lines + the "2688 blocks" line + the
			#    "312 error(s)" line out of R.err$$
			# If there are any lines left then they must be errors
			# This is messy but since cpio reports these as errors,
			# this is the best we can do.
			egrep -v '(same age or newer)|([0-9]* blocks)|([0-9]* error\(s\))' /tmp/R.err$$ > /tmp/R.ovw$$
			ERR=`wc -l /tmp/R.ovw$$ | tr -s " " " " | cut -f2 -d" "`
			if [ $ERR -gt 0 ]
			then
				cat /tmp/R.ovw$$ >&2
				echo "$ERR error(s)\n\n" >&2
				echo "An error was encountered while reading from $INPUT_DRIVE." >&2
				echo "Please be sure the archive is inserted properly, and wait" >&2
				echo "for notification before removing it." >&2

				exit 1
			else
				rm -f /tmp/R.chk$$ /tmp/R.err$$ /tmp/R.ovw$$
				message -d "The restore is now finished." >&2
			fi
		else
			echo "An error was encountered while reading from $INPUT_DRIVE." >&2
			echo "Please be sure the archive is inserted properly, and wait" >&2
			echo "for notification before removing it." >&2
			exit 1
		fi
	else
		NOMATCH=
		for i in $PATTERNS
		do
			A=`echo "$i" | sed 's;\";;g
				s;\.;\\\.;g
				s;\*;\.\*;g'`
			NUMERR=`cat /tmp/R.chk$$ | sed -n "\;$A;p" 2>/dev/null | wc -l`
			if [ "$NUMERR" -eq 0 ] 
			then
				NOMATCH="$NOMATCH\n\t$i"
			fi
		done
		if [ -n "$NOMATCH" ]
		then
			echo "The following files and directories were not found.\n${NOMATCH}\n\n" | pg # >&2
		fi
		rm -f /tmp/R.chk$$ /tmp/R.err$$
		message -d "The restore is now finished." >&2
	fi
	exit 0
	;;
i)	
	message -c "Insert the backup archive into $INPUT_DRIVE." >&2
	if [ "$?" -ne 0 ]
	then
		echo "You have cancelled the restore" >&2
		exit 1
	fi
	message -d "Reading the backup archive.\nPlease do not remove." >&2
	trap '' 1 2 3 9 15
	xtract "${CPIOFLGS}" /tmp/flp_index  $DEV > /tmp/cplst$$
	if [ $? -ne 0 ]
	then
		exit 1
	fi
	cat /tmp/flp_index | sed '/^BLOCK/d' 
	rm -f /tmp/cplst$$ /tmp/flp_index
	;;
*)	exit_msg "$0: Invalid type $type" >&2 ;;
esac
