#ident	"@(#)crash:i386at/cmd/crash/ldsysdump.sh	1.6"
# load a system memory image dump from tape or floppy to a file

# shell variables used:
#
#       DEV     "f" = floppy; "t" = tape
#
#       IF      input device file used by dd
#       BS      block size used by dd, in 512 byte units
#       COUNT   number of blocks per disk/tape
#
#       NB      number of BS size blocks on tape/disk
#       N       number of BS size blocks of memory to copy


if [ $# -ne 1 ]
then
	echo 'usage: /etc/ldsysdump file'
	exit
fi

while :
do
	echo '\nIs the dump on:'
	echo '  1 - low density 5.25" (360K) diskettes'
	echo '  2 - high density 5.25" (1.2M) diskettes'
	echo '  3 - low density 3.5" (720K) diskettes'
	echo '  4 - high density 3.5" (1.44M) diskettes'
	if [ -r /dev/rmt/ctape1 ]
	then
		echo '  t - Cartridge tape'
	fi
	echo '  n - no, QUIT'
	echo '> \c'
	read ans
	case $ans in
	  1 )   BS=18 NB=40     DEV=f ; break ;;
	  2 )   BS=30 NB=80     DEV=f ; break ;;
	  3 )   BS=18 NB=80     DEV=f ; break ;;
	  4 )   BS=36 NB=80     DEV=f ; break ;;
	  t )   IF=/dev/rmt/ntape1     BS=1024 NB=100  DEV=t ;
		if [ -r $IF ]; then break; fi ;;
	  n )   exit ;;
	esac
	echo '???'
done

if [ "$DEV" = "f" ]
then
/sbin/flop_num
if [ $? -gt 1 ]
then
	while true
	do
		echo "\nThis system has two floppy drives.\n\
Strike ENTER to load dump from drive 0\n\
or 1 to load dump from drive 1.  \c"
		read ans
		if [ "$ans" = 1 ]
		then
			IF=/dev/rdsk/f1t
			break
		elif [ "$ans" = "" -o "$ans" = 0 ]
		then
			IF=/dev/rdsk/f0t
			break
		fi
	done
else
	IF=/dev/rdsk/f0t
fi
fi

# while :
# do
#	echo 'How many megabytes of memory image do you want to load?'
#	echo 'Enter decimal integer or "q" to quit. > \c'
#	read ans
#	case $ans in
#	  q )   exit ;;
#	esac
#	N=`expr \( $ans \* 2048 + $BS - 1 \) / $BS`
#	case $? in
#	  0 )   break;;
#	esac
#	echo '???'
# done

SKIP=0
COUNT=$NB

echo 'Insert \c' >&2
case $DEV in
	  f )   echo 'diskette \c' >&2 ;;
	  t )   echo 'tape cartridge \c' >&2 ;;
esac
echo 'and press return to load it, or enter q to quit. > \c' >&2
read ans
case $ans in
	q )   exit ;;
esac

if [ $DEV = 't' ]
then 
	tapecntl -f 512
	tapecntl -w $IF
	N=`/sbin/memsize /dev/rmt/ctape1`
else
	N=`/sbin/memsize $IF`
fi

N=`expr \( \( $N + 511 \) / 512 + $BS - 1 \) / $BS`

OLIM=`ulimit`
ulimit `expr $N \* $BS`

{   while [ $N != 0 ]
    do

	echo 'Wait.' >&2

	if [ $COUNT -gt $N ]
	then
		COUNT=$N
	fi
	echo dd if=$IF of=${1} bs=${BS}b count=$COUNT oseek=$SKIP >&2
	dd if=$IF of=${1} bs=${BS}b count=$COUNT oseek=$SKIP 2>/tmp/ldsysdump.out
	if [ $? != 0 ]	
	then
		cat /tmp/ldsysdump.out
		case $DEV in
			t )	pfmt -s nostd -g $CAT:999 "Please check the tape and press return to continue > " ;;
			f )	pfmt -s nostd -g $CAT:999 "Please check the disk and press return to continue > " ;;
			esac
			read ans 
			continue
	fi
	cat /tmp/ldsysdump.out
	SAVIFS=$IFS
	IFS='+'
	read num_recs x < /tmp/ldsysdump.out
	IFS=$SAVIFS
	if [ `expr $num_recs + 1` -lt $COUNT ] 
	then
		case $DEV in
			t ) pfmt -s nostd -g $CAT:999 "Insert next tape if done reading this one and press return > " 
			    read ans
			    tapecntl -w $IF ;;
			f )	pfmt -s nostd -g $CAT:999 "Insert next disk if done reading this one and press return > " 
			    read ans ;;
		esac
		continue
	fi
	N=`expr $N - $COUNT`
	SKIP=`expr $SKIP + $COUNT`
    done
}

rm /tmp/ldsysdump.out
ulimit $OLIM

echo "System dump copied into $1.  Use crash(1M) to analyze the dump."
