#!/sbin/sh
#	copyright	"%c%"


#ident	"@(#)initpkg:i386/cmd/initpkg/dumpsave.sh	1.16.1.3"

cat <<\END >dumpsave
#!/sbin/sh

#	Copyright (c) 1984, 1986, 1987, 1988, 1989 AT&T
#	  All Rights Reserved

#	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF AT&T
#	The copyright notice above does not evidence any
#	actual or intended publication of such source code.

# This script should be invoked with the folowing arguments:
# 	dumpsave path [/dev/swap] skip dumpsize swapsize
#

if [ -z "$LC_ALL" -a -z "$LC_MESSAGES" ]
then
	if [ -z "$LANG" ]
	then
		LNG=`defadm locale LANG 2>/dev/null`
		if [ "$?" != 0 ]
		then LANG=C
		else eval $LNG
		fi
	fi
	LC_MESSAGES=/etc/inst/locale/$LANG
	export LANG LC_MESSAGES
fi
LABEL="UX:$0"

CAT=uxrc; export CAT

# shell variables used:
#
#       DEV     "f" = floppy; "t" = tape
#
#       OF      output device file used by dd
#       BS      block size used by dd, in 512 byte units
#       COUNT   number of blocks to be copied by dd
#       SKIP    number of blocks for dd to skip
#
#       NB      number of BS size blocks on tape/disk
#       N       number of BS size blocks of memory to copy
#	Z	normal/compressed dump
#
Z='n'
while :
do
	pfmt -s nostd -g $CAT:139 '\nDo you want to save it on:\n' 2>&1
	pfmt -s nostd -g $CAT:140 '  1 - low density 5.25" (360K) diskettes\n' 2>&1
	pfmt -s nostd -g $CAT:141 '  2 - high density 5.25" (1.2M) diskettes\n' 2>&1
	pfmt -s nostd -g $CAT:142 '  3 - low density 3.5" (720K) diskettes\n' 2>&1 
	pfmt -s nostd -g $CAT:143 '  4 - high density 3.5" (1.44M) diskettes\n' 2>&1
	if [ -w /dev/rmt/ntape1 ]
	then
		pfmt -s nostd -g $CAT:144 '  t - Cartridge tape\n' 2>&1
	fi
	pfmt -s nostd -g $CAT:158 '  f - file [/dumpfile]\n' 2>&1
	pfmt -s nostd -g $CAT:145 '  n - no, QUIT\n'
	echo '> \c'
	read ans
	case $ans in
	  1 )   BS=18 NB=40     DEV=f ; break ;;
	  2 )   BS=30 NB=80     DEV=f ; break ;;
	  3 )   BS=18 NB=80     DEV=f ; break ;;
	  4 )   BS=36 NB=80     DEV=f ; break ;;
	  t )   OF=/dev/rmt/ntape1     BS=1024 NB=100  DEV=t ;
		if [ -w $OF ]; then break; fi ;;
	  f )	BS=1000 DEV=d ;
		pfmt -s nostd -g $CAT:145 'compress dump (y/n) ?\n'
		read ans2
		if [ "$ans2" = "y" ]
		then
			echo "dump will be compressed"
			Z='y'
			fname='/dumpfile.Z'
		else
			echo "dump will not be compressed"
			Z='n'
			fname='/dumpfile'
		fi
		size=`expr \( ${3} + 511 \) / 512 `

		/sbin/df / | cut -f2 -d: > /tmp/dfout
		read free rest < /tmp/dfout

		if [ $size  -gt $free ]
		then
			pfmt -s nostd -g $CAT:159 'not enough space in root file system for the dump'
			continue
		fi
		while test -f $fname || test -d $fname
		do
			pfmt -s nostd -g $CAT:160 "%s already exists; Enter new file name (in root file system): " $fname
			read fname
		done
		OF=$fname ;
		COUNT=$NB
		NB=`expr \( \( ${3} + 511 \) / 512 + $BS - 1 \) / $BS`
		break ;;
	  n )   
		echo MARK | /sbin/dd of=${1} skip=${2} conv=sync >/dev/null 2>&1
		swap -a ${1} ${2} ${4}
		exit 0 ;;
	esac
	echo '???'
done

if [ "$DEV" = "f" ]
then
/sbin/flop_num
if [ $? = 2 ]
then
	while [ 1 ]
	do
		pfmt -l $LABEL -s info -g $CAT:146 "\nThis system has two floppy drives.\nStrike ENTER to save the dump in drive 0\nor 1 to save the dump in drive 1.  "
		read ans
		if [ "$ans" = 1 ]
		then
			OF=/dev/rdsk/f1t
			break
		elif [ "$ans" = "" -o "$ans" = 0 ]
		then
			OF=/dev/rdsk/f0t
			break
		fi
	done
else
	OF=/dev/rdsk/f0t
fi
fi

q=`gettxt $CAT:147 "q"`

SKIP=${2}
COUNT=$NB
N=${3}
N=`expr \( \( $N + 511 \) / 512 + $BS - 1 \) / $BS`

DD_ERR=0

if [ $DEV != 'd' ]
then
	case $DEV in
		f )	pfmt -s nostd -g $CAT:150 "Insert %s and press return, or enter %s to quit. > " "diskette" $q 2>&1 ;;
		t )	pfmt -s nostd -g $CAT:150 "Insert %s and press return, or enter %s to quit. > " "tape cartridge" $q 2>&1 ;;
	esac
	read ans
	case $ans in
		$q )  
			echo MARK | /sbin/dd of=${1} skip=${2} conv=sync >/dev/null 2>&1
			swap -a ${1} ${2} ${4} 
			exit 0 ;;
	esac
fi

# rewind the tape
	if [ $DEV = 't' ]
		then 
			tapecntl -f 512
			tapecntl -w $OF
	fi
while [ $N -gt 0 ]
do
	if [ $SKIP -gt 0 ]
	then
		case $DEV in
			f )	pfmt -s nostd -g $CAT:9999 "If disk is full, Insert another %s and press return > " "diskette" $q 2>&1 ;;
			t )	pfmt -s nostd -g $CAT:9999 "If tape is full, Insert another %s and press return > " "tape cartridge" $q 2>&1 ;;
		esac
		read ans
	# rewind the tape
		if [ $DEV = 't' ]
			then tapecntl -w $OF
		fi
	fi
	if [ $COUNT -gt $N ]
	then
		COUNT=$N
	fi
	if [ $DD_ERR != 0 ]
	then
		pfmt -s nostd -g $CAT:9999 "Do you want to continue saving the dump (y or n) ? "
			read ans
			if [ "$ans" = 'n' ]
			then
				echo MARK | /sbin/dd of=${1} skip=${2} conv=sync >/dev/null 2>&1
				swap -a ${1} ${2} ${4} 
				exit 0 ;
			fi
		if [ $DD_ERR = 5 ]
		then
			case $DEV in
				f )	pfmt -s nostd -g $CAT:9999 "If disk is full, Insert another %s and press return > " "diskette" $q 2>&1 ;;
				t )	pfmt -s nostd -g $CAT:9999 "If tape is full, Insert another %s and press return > " "tape cartridge" $q 2>&1 ;;
			esac
			read ans
# rewind the tape
			if [ $DEV = 't' ]
				then tapecntl -w $OF
			fi
		fi
		DD_ERR=0
	fi

	pfmt -s nostd -g $CAT:151 'Wait.\n'

	if [ "$Z" = 'y' ]
	then
		echo "dd if=${1} bs=${BS}b count=$COUNT skip=$SKIP | compress -c | dd of=$OF bs=${BS}b"
		dd if=${1} bs=${BS}b count=$COUNT skip=$SKIP | /usr/bin/compress -c | dd of=$OF bs=${BS}b
	else
		echo dd if=${1} of=$OF bs=${BS}b count=$COUNT skip=$SKIP 
		dd if=${1} of=$OF bs=${BS}b count=$COUNT skip=$SKIP 
	fi
	if [ $? != 0 ]
	then
		DD_ERR=$1
		if [ "$DEV" = "d" ]
		then
			pfmt -s nostd -g $CAT:161 "write to %s failed \n" $OF
			pfmt -s nostd -g $CAT:162 "Enter new file name : "
			read OF
			while test -f $OF || test -d $OF
			do
				pfmt -s nostd -g $CAT:160 "%s already exists; Enter new file name (in root file system): " $fname
				read OF
			done
		fi
		continue
	fi
	N=`expr $N - $COUNT`
	SKIP=`expr $SKIP + $COUNT`
done
echo MARK | /sbin/dd of=${1} skip=${2} conv=sync >/dev/null 2>&1
swap -a ${1} ${2} ${4}
if [ "$DEV" != "d" ]
then
pfmt -l $LABEL -s info -g $CAT:154 '\nDone.  Use /etc/ldsysdump to copy dump from tape or diskettes\n'
else
pfmt -s nostd -g $CAT:163 "Dump saved in file %s\n" $OF
fi
pfmt -s nostd -g $CAT:155 'Press return to continue >'
read ans
END


