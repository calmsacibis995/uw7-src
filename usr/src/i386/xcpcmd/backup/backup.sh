#!/sbin/sh
#	copyright	"%c%"

#ident	"@(#)xcpbackup:i386/xcpcmd/backup/backup.sh	1.1.2.8"
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

	D_MAJ=`ls -l $DEV | sed 's/  */;/g' | cut -d";" -f5 | sed 's/,//'`
	while read N D T
	do
		MAJ=`ls -l $D | sed 's/  */;/g' | cut -d";" -f5 | sed 's/,//'`
		[ $MAJ -eq $D_MAJ ] && {
			echo $N $DEV $T >/tmp/Xtp_use$$
			break
		}
	done </tmp/Xtape$$
}

dofind() {
	find $1 $2 -print | while read file
	do
		[ -f "$file" ] && {
			du -a $file
			continue
		}
		[ -d "$file" ] && echo "D\t$file"
	done >> /tmp/FILE$$
}

# main()
#	Backup script 
#	Options:
#	c - complete backup
#	p - incremental ("partial") backup
#	h - backup history
#	u "user1 [user2]" - backup users (use "all" to backup all users)
#	t - backup done to tape; only used if backing up to tape
#	d - special device name; defaults to /dev/rdsk/f0q15d
#	f "<files>" - backup by file name (argument must be in quotes)

USAGE="Usage:\t$0 -d "device" -c | -p [ -t ] [ -u [user] | -f <files> ]\n\t$0 -h"
TAPE="cartridge tape"
MEDIA="floppy disk"
CPIO=cpio
CPIOFLGS="-ocB"
DEV=""
BTYPE=""
BACKUP=/etc/Backup
IGNORE=/etc/Ignore
DIR=

clean_up() {
	rm -f /tmp/FFILE$$ /tmp/FILE$$ /tmp/VFILE$$ /tmp/flp_index /tmp/X*$$
}

# clean_up before aborting backup.
trap "clean_up;\
      echo \"You have cancelled the Backup.\"; trap 0; exit " 1 2 3 9 15

# clean-up after a normal exit.
trap " clean_up; trap 0; exit " 0

while getopts tcphu:f:d: c
do
	case $c in
	c)	type=c
		BTYPE="Complete System"
		;;
	p)	type=p
		BTYPE="Incremental System"
		;;
	d)	DEV="$OPTARG"
		;;
	t)	MEDIA="$TAPE"
		CPIOFLGS="-oc -C 10240 "
		TFLAG=YES
		;;
	h)	type=h;
		;;
	u)	[ -n "$FILES" ] &&
			exit_msg  "$0: -u and -f options can't be used together"
		type=u
		user="$OPTARG"
		BTYPE="User"
		;;
	f)	[ -n "$user" ] &&
			exit_msg "$0: -u and -f options can't be used together"
		type=f
		FILES="$OPTARG"
		BTYPE="Selective"
		;;
	\?)	echo "backup: $USAGE"
		exit 0
		;;
	*)	exit_msg ;;
	esac
done

case "$type" in

  "")	MSG="$0:\tspecify either c or p option for complete or partial backup"
	MSG="$MSG\n\tor use the h option for backup history"
	exit_msg "$MSG" ;;
   h)	MSG="No complete backup has been done."
	[ -s /etc/.lastbackup ] &&
		MSG="Last complete backup done on `cat /etc/.lastbackup`"
	echo "$MSG"
	MSG="No incremental backup has been done."
	[ -s /etc/.lastpartial ] &&
		MSG="Last incremental backup done on `cat /etc/.lastpartial`"
	echo $MSG
	exit 0
	;;
esac

shift `expr $OPTIND - 1`

# $DEV, if set, must be a  readable file
[ "$DEV" -a  ! -r "$DEV" ] && 
	exit_msg "$0: $DEV - no such device"

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

# If device node is specified on the command line, determine if it is
# floppy disk, or cartridge tape or neither of the two. Set DEVNAME,
# TYPE and adjust MEDIA if TYPE=qtape and TFLAG is not set.

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

	    "")	    exit_msg "$0: $DEV is not a supported backup medium" ;;
	     *)	    exit_msg "$0: device type for $DEV is unknown" ;;
	esac

	[ "$TFLAG" -a "$MEDIA" != "$TAPE" ] &&
		exit_msg "$0: -t option can not be used for the device $DEV"

}

# if DEV is not set, ask the user to make a selection from a menu of tape
# nodes installed on the system.
[ ! "$DEV"  -a  "$MEDIA" = "$TAPE" ] && {

	NUM_TP_DRIVES=`grep qtape /tmp/Xdev$$ | wc -l `
	[ $NUM_TP_DRIVES -eq 0 ] &&
		exit_msg "No tape entry in the database /etc/device.tab"

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
}

#  For tape, reset CPIOFLAG if the tape is scsi.
[ "$MEDIA" = "$TAPE" ] && {
	scsi=`devattr $DEVNAME scsi`
	[ $? = 0  -a "$scsi" = "true" ] && CPIOFLGS="-oc -C 65536 "
}

MESSAGE="You may remove the $MEDIA. To exit, strike 'q' followed by ENTER.
To continue, insert $MEDIA number %d and strike the ENTER key. "

UID=`id | cut -c5`
if [ "$UID" != "0" ]
then
	RNAME=`id | cut -d\( -f2 | cut -d\) -f1`
	if [ "$type" = "c" -o "$type" = "p" ]
	then
		echo "\"$RNAME\" may only select 'Backup History' \
or 'Private Backup'."
		exit 1
	fi
	if [ \( "$type" = "u" -a "$user" = "all" \) ]
	then
		echo "\"$RNAME\" may only select 'Backup History' \
or 'Private Backup'."
		exit 1
	fi
fi

case $type in
	c)
#	handle complete backup
		TIR=/
		file2=/etc/.lastbackup
		;;
	p)
#		set up for an incremental update
#		assumption: date IN the file is the same as date OF the file
		if [ ! -s /etc/.lastbackup ]
		then
			echo "A complete backup up must be done before the Incremental backup."
			exit 1
		fi
		if [ ! -s /etc/.lastpartial ]
		then 
			NEWER="-newer /etc/.lastbackup"
		else 
			NEWER="-newer /etc/.lastpartial"
		fi
		file2=/etc/.lastpartial
		TIR=/
		;;
	f)
		DIR="$FILES"
		NEWER=
		TIR=
		;;
	u)
		DIR=
		if [ "$user" = "all" ]
		then
			user="`pwdmenu`"
			if [ "Name=N O N E" = "$user" ]
			then
			   echo "There are no users to backup"
			   exit 1
			fi
			for i in `pwdmenu`
			do
				T=`echo $i | cut -d= -f2`
				T=`grep "^$T:" /etc/passwd | cut -d: -f6`
				#ignore users with home "/"
				if [ "$T" != "/" ]  
				then
					DIR="$DIR $T"
				fi
			done
			TIR=/usr
		else
			for i in $user
			do
				TDIR=`grep "^$i:" /etc/passwd | cut -d: -f6`
				if [ -z  "$TDIR" ]
				then
					echo "$i doesn't exist"
					continue
				else
					DIR="$DIR $TDIR"
				fi
			done
			if [ -z "$DIR" ]
			then
				echo "No users to back up."
				exit 1
			fi
			TIR=/usr
		fi
		;;
	*)	# THIS CASE SHOULDN'T BE REACHED
		exit_msg "$0: Invalid type $type"
		;;
esac


#echo "DEBUG: DEV=$DEV DEVNAME=$DEVNAM	 hit return->"; read XXX </dev/tty
message -d "Calculating approximate number of $MEDIA(s) required. Please wait."

#  Compute Blocks, number of floppies etc ..
>/tmp/FILE$$
if [ "$type" = "c" -o "$type" = "p" ]
then
	if [ -s "$BACKUP" ]
	then
		> /tmp/FFILE$$
		for file in `cat $BACKUP`
		do
			if [ -f "$file" ]
			then
				du -a $file >>/tmp/FFILE$$
			else
				DIR="$file $DIR"
			fi
		done 
	fi
	if [ -z "$DIR"  -a ! -s /tmp/FFILE$$ ]
	then
		DIR=/
	fi
fi

dofind "$DIR" "$NEWER"

if [ "$type" = "c" -o "$type" = "p" ]
then
	if [ -s "$IGNORE" ]
	then
		cat $IGNORE | while read file
		do
			if [ -n "$file" ]
			then
				grep -v "^[0-9D]*	$file" /tmp/FILE$$ >/tmp/VFILE$$
				mv /tmp/VFILE$$ /tmp/FILE$$
			fi
		done
	fi
	cat /tmp/FFILE$$ >>/tmp/FILE$$ 2>/dev/null
fi

grep -v "^D" /tmp/FILE$$ > /tmp/VFILE$$
# cp /tmp/FILE$$ /tmp/flp_index
sed "s/\	\.\//	/" /tmp/FILE$$ > /tmp/flp_index
if [ ! -s /tmp/VFILE$$ ]
then
	if [ "$type" != "u" -a "$type" != "f" ]
	then
		if [ ! -s /etc/.lastpartial ]
		then 
			set `cat -s /etc/.lastbackup`
		else 
			set `cat -s /etc/.lastpartial`
		fi
		message -d "There are no new files since \
the last Incremental Backup on $2 $3.  Therefore no Backup will be done."
	else
		echo  "There are no files to be backed up."
	fi
	exit 0
fi

BLOCKS=`cut -f1 < /tmp/VFILE$$ | sed -e '2,$s/$/+/' -e '$s/$/p/'| dc`

# If there are only directories to back up, BLOCKS could be 0.  Make sure
# the message tells the user at least 1 media is needed.

if [ $BLOCKS -eq 0 ]
then
	BLOCKS=1
fi
cut -f2 /tmp/FILE$$ > /tmp/VFILE$$

# Block maxs are:
# 702 for 360K, 1422 for 720K, 2370 for 1.2MB and 2844 for 1.44MB floppies
# 131072 for the 60MB cartridge tape
# 257000 for the 125MB cartridge tape. This also allows for cpio overhead.

if [ "$MEDIA" = "$TAPE" ]
then
	CTAPEA=`expr \( $BLOCKS / 131072 \) + \( 1 \& \( $BLOCKS % 131072 \) \)`
	CTAPEB=`expr \( $BLOCKS / 257000 \) + \( 1 \& \( $BLOCKS % 257000 \) \)`
	CTAPEC=`expr \( $BLOCKS / 514000 \) + \( 1 \& \( $BLOCKS % 514000 \) \)`
	TIME=`expr ${CTAPEA} \* 30`
	echo /tmp/flp_index > /tmp/FILE$$
	cat /tmp/VFILE$$ >> /tmp/FILE$$ 2>/dev/null
	echo "BLOCKS=$BLOCKS" >> /tmp/flp_index
	rm -f /tmp/VFILE$$
	message "The backup will need approximately:\n\
$CTAPEA cartridge tape(s) for a 60 MB drive or \n\
$CTAPEB cartridge tape(s) for a 125 MB drive \n\
$CTAPEC cartridge tape(s) for a 320 MB drive \n\
and will take no more than $TIME minute(s).\n\n\
Please insert the first cartridge tape.  Be sure to number the cartridge tape(s) \
consecutively in the order they will be inserted."
else
	FP5D9=`expr \( $BLOCKS / 690 \) + \( 1 \& \( $BLOCKS % 690 \) \)`
	FP5H=`expr \( $BLOCKS / 2360 \) + \( 1 \& \( $BLOCKS % 2360 \) \)`
	FP3H=`expr \( $BLOCKS / 2830 \) + \( 1 \& \( $BLOCKS % 2830 \) \)`
	FP3Q=`expr \( $BLOCKS / 1410 \) + \( 1 \& \( $BLOCKS % 1410 \) \)`
	TIME=`expr \( ${FP5D9}0 / 15 \) + \( 1 \& \( ${FP5D9}0 % 15 \) \)`
	echo /tmp/flp_index > /tmp/FILE$$
	cat /tmp/VFILE$$ >> /tmp/FILE$$ 2>/dev/null
	echo "BLOCKS=$BLOCKS" >> /tmp/flp_index
	rm -f /tmp/VFILE$$
	message "The backup will need approximately:\n\
$FP5H formatted 1.2MB 5.25\" floppy disk(s) or\n\
$FP5D9 formatted 360KB 5.25\" floppy disk(s) or\n\
$FP3H formatted 1.44MB 3.5\" floppy disk(s) or\n\
$FP3Q formatted 720KB 3.5\" floppy disk(s).\n\
and will take no more than $TIME minute(s).\n\n\
Please insert the first floppy disk.  The floppy disk(s) you are using \
for the backup MUST be formatted.  Be sure to number the floppy disks \
consecutively in the order they will be inserted."
fi

[ "$FLOP_DRIVE" ] && {
	while true
	do
		>$DEV 2>/dev/null
		rc=$?
		[ $rc -ne 0 ] && {
			echo "Insert the $MEDIA in drive $FLOP_DRIVE and hit ENTER->\c"
			read ans
			continue
		}
		break
	done
}
echo "\n$BTYPE backup in progress\n\n\n"

cat /tmp/FILE$$ | ${CPIO} ${CPIOFLGS} -M"$MESSAGE" -O $DEV
err=$?
if [ "$err" = "4" ] 
then
	message -d "You have cancelled the Backup to $MEDIA(s)."
elif [ "$err" != "0" ] 
then
	if [ "$MEDIA" = "$TAPE" ]
	then
		message -d "An error was encountered while writing to the ${MEDIA}. \
Please be sure the $MEDIA is inserted properly, and wait for the notification \
before removing it."
	else
		message -d "An error was encountered while writing to the ${MEDIA}. \
Please be sure all your $MEDIA(s) are formatted, and wait for the notification \
before removing them."
	fi
else
	if [ "$type" = "c" ]
	then
		message -d "The Complete Backup is now finished."
	elif [ "$type" = "p" ]
	then
		set `cat -s /etc/.lastbackup`
		eval M=$2 D=$3
		if [ "$NEWER" = "-newer /etc/.lastbackup" ]
		then 
			set `cat -s  /etc/.lastbackup`
		else 
			set `cat -s  /etc/.lastpartial`
		fi
		message -d  "The incremental backup of the files that have changed \
since the last backup, on $2 $3, is now finished.\n\n\
In order to restore the system totally, first restore the last \
complete backup dated $M $D, and then restore the incremental backup(s) that \
you have performed since $M $D starting with earliest one and \
ending with the one you have just finished."
	else
		message -d "Backup is now completed."
	fi
	if [ "$TIR" = "/" ]
	then
		if [ "$file2" = "/etc/.lastbackup" ]
		then 
			rm -f /etc/.lastpartial
		fi
		date > $file2
	fi
fi
