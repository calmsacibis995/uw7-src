:
#ident	"@(#)OSRcmds:diskcp/diskcp.sh	1.1"
#	@(#) diskcp.sh 23.2 91/08/29 
#
#	Copyright (C) 1986-1991 The Santa Cruz Operation, Inc.
#		All Rights Reserved.
#	The information in this file is provided for the exclusive use of
#	the licensees of The Santa Cruz Operation, Inc.  Such users have the
#	right to use, modify, and incorporate this code into other products
#	for purposes authorized by the license agreement provided they include
#	this notice and the associated copyright notice with any such product.
#	The information in this file is provided "AS IS" without warranty.
#
# Floppy disk copy routine
#
# Diagnostics:
#	Returns 1 on syntax errors, 2 on interrupts
#	
#   	BLSZ is block size arg for dd to use
#	DISK0 is always where the source comes from 
#	DRIVE0 & DRIVE1 specify source and target devices
#	FORMAT is the flag for the argument
#	FTRY is to make sure we don't get stuck in endless format loop
#	SRC is always where the target copy comes from
#	SUM is the flag for the argument
#	SSUM is the source sum and TSUM is the target sum
#	SWAP is the flag for -r (second disk as source device)
#	TARGET is always where the target copy goes to
#	TYPE is the drive type: 48 | 96ds9 | 96ds15 | 135ds9 | 135ds18
#	TARGDRV is the target drive. If dual floppy drives, it is DRIVE1,
#				     else it is DRIVE0.

PATH=/bin:/usr/bin:/etc
TEMP=/tmp/disk$$
BLSZ=10k		FORMAT=no	SWAP=
DISK0=/dev/rinstall	FTRY=0		TARGET=
DISK1=/dev/rinstall1	SRC=$TEMP	TSUM=
DRIVE0=0		SUM=		TYPE=
DRIVE1=1		SSUM=		
DUALDISK=

usage() {
	echo "
Usage:
diskcp [-f] [-d] [-r] [-s] [-u] [-48ds9 | -96ds15 | -135ds9 | -135ds18 |
		-48 | -96 | -135 | 360 | 720 | 1.2 | 1.44 ]
	-f	 	format target disks before copy
	-d	 	use dual drives for copy
	-r		use second drive as source drive
	-s	 	sum source and target disks
	-u		print this usage message
	-48ds9	 	copy low density 48tpi 5.25 inch disks (360KB)
	-48	 	copy low density 48tpi 5.25 inch disks (360KB)
	-360	 	copy low density 48tpi 5.25 inch disks (360KB)
	-96ds15	 	copy quad density 96tpi 5.25 inch disks (1.2MB)
	-96	 	copy quad density 96tpi 5.25 inch disks (1.2MB)
	-1.2	 	copy quad density 96tpi 5.25 inch disks (1.2MB)
	-135ds9	 	copy high density 135tpi 3.5 inch disks (720KB)
	-135	 	copy high density 135tpi 3.5 inch disks (720KB)
	-730	 	copy high density 135tpi 3.5 inch disks (720KB)
	-135ds18	copy quad density 135tpi 3.5 inch disks (1.44MB)
	-1.44		copy quad density 135tpi 3.5 inch disks (1.44MB)"

	exit 1
}

#	Trap interrupts, etc for a clean exit.

trap "rm -f $TEMP; echo $0 interrupted; exit 2" 1 2 3 15

#
# FUNCTION DEFINITIONS
#
# Prompt for yes or no answer - returns non-zero for no
getyn() {
	while	echo "\n$* (y/n) \c">&2
	do	read yn rest
		case $yn in
		[yY])	return 0 			;;
		[nN])	return 1			;;
		*)	echo "Please answer y or n" >&2	;;
		esac
	done
}

# Prompt with mesg, return non-zero on q
prompt() {
	while	echo "\n$* \nPress <Return> to proceed or type q to quit: \c" >&2
	do	read cmd
		case $cmd in
		Q|q)	rm -f $TEMP; exit 1	;;
		"")	break			;;
		*)	continue		;;
		esac
	done
}

#	Send all echos to stderr for the duration of this script.
exec 1>&2

#	Argument processing
while	case $1 in
	"")		break			;;
	-s)		SUM=1			;;
	-48|-48ds9|-360)
			BLSZ=9k
			TYPE=5d9t		;;
	-96|-96ds15|-1.2)
			BLSZ=15k
			TYPE=5ht		;;
	-135|-135ds9|-720)
			BLSZ=9k
			TYPE=3dt		;;
	-135ds18|-1.44)
			BLSZ=18k
			TYPE=3ht		;;
	-d)		DUALDISK=1		;;
	-f)		FORMAT=yes		;;
	-r)		SWAP=1
			DRIVE0=1
			DRIVE1=0		;;
	*)		usage	 		;;
	esac
do	shift
done

[ "$FORMAT" = yes -a -z "$TYPE" ] && {
	echo "Cannot format disks without specific drive type"
	usage
}

[ "$SWAP" -a -z "$TYPE" ] && { echo "
	You must specify drive type when using the -r flag"
	usage
	}

[ "$TYPE" ] && DISK0=/dev/rdsk/f$DRIVE0$TYPE DISK1=/dev/rdsk/f$DRIVE1$TYPE

TARGET=$DISK0
TARGDRV=$DRIVE0
[ $DUALDISK ] && {
	TARGET=$DISK1 SRC=$DISK0
	TARGDRV=$DRIVE1
}

#	If single disk, we need to put the floppy on the hard disk.
#	If dual disk, simply prompt with the right drive name.

while	prompt "Please insert the source disk in drive $DRIVE0."

	trap "rm -f $TEMP; exit 1" 1 2 3 15
	[ "$SUM" ] && {
		SSUM="`dd if=$DISK0 bs=$BLSZ | sum -r`" || {
			echo "Sum of source disk failed."
			SSUM="not available"
		}
		echo "\nSum of source floppy is -- $SSUM --\n"
	}

	[ "$DUALDISK" ] || `dd bs=$BLSZ if=$DISK0 of=$TEMP` || { 
		if [ "$TYPE" ]; then
			echo "Error, please try again."
		else
			# /dev/install cannot detect 96ds9 5.25 inch disks.
			echo "\nSpecify device type and try again"
			usage
		fi
		continue
	}

do	while prompt "Please insert the target disk in drive $TARGDRV."

	#	If the disk was unformatted, and FORMAT=no, format it anyway.
	do	[ "$FORMAT" = yes ] && {
			format $TARGET
			FTRY=1
		}
		until dd bs=$BLSZ if=$SRC of=$TARGET
		do
			case $? in
			2)	if [ "$FTRY" -eq 1 ]; then
					# already formatted, still failed
					echo "System error." 
					continue 2
				elif [ "$TYPE" ]; then
					# try formatting if device is set
					format -f $TARGET
					FTRY=1
				else 	# /dev/install cannot detect 96ds9
					# 5.25 inch disks.
					echo "Specify device type and try again"
					usage
				fi		;;
	
			*)	echo "System error, please try again"
				continue 2	;;
			esac
		done
	
		[ "$SUM" ] && {
			TSUM="`dd if=$TARGET bs=$BLSZ | sum -r`" || {
				echo "Sum of target disk failed."
				TSUM="not available"
			}
			echo "Sum of target floppy is -- $TSUM --\n"
			[ "$SSUM" != "$TSUM" ] && echo "
WARNING:  Sum of target differs from sum of source."
		}
		echo "Copy complete."

		#	If dual drives start from beginning again (no 2nd copy)

		[ "$DUALDISK" ] && break

		#	Copy's on disk, ask if they want a another run

		getyn "Do you want another copy of this source disk?" || break

	done

	#	Should whole process be repeated with new source

	getyn "Do you want to copy a different source disk?" || {
		rm -f $TEMP
		exit 0
	}
done
