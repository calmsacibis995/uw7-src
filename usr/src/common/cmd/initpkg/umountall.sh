#!/sbin/sh
#ident	"@(#)initpkg:common/cmd/initpkg/umountall.sh	1.4.20.1"

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

usage()
{
FARG=`gettxt $CAT:57 "FSType"`
USAGE="umountall [-F $FARG] [-k] [-l|-r] "
pfmt -l $LABEL -s action -g $CAT:4 "Usage: %s\n" "$USAGE"
exit 2
}
FSTAB=/etc/vfstab
FSType=
kill=
FCNT=0
CNT=0

while getopts ?rlkF: c
do
	case $c in
	r)	RFLAG="r" export RFLAG; CNT=`/usr/bin/expr $CNT + 1`;;
	l)	LFLAG="l" export LFLAG; CNT=`/usr/bin/expr $CNT + 1`;;
	k) 	kill="yes" export kill;;
	F)	FSType=$OPTARG export FSType
		case $FSType in
		?????????*) 
			pfmt -l $LABEL -s error -u $CAT:60 "FSType %s exceeds 8 characters\n" $FSType
			exit 2
		esac;
		FCNT=`/usr/bin/expr $FCNT + 1`;;
	\?)	usage
		;;
	esac
done
shift `/usr/bin/expr $OPTIND - 1`
if test $FCNT -gt 1
then
	pfmt -l $LABEL -s error -g $CAT:59 "more than one FSType specified\n"
	exit 2
fi
if test $CNT -gt 1
then
	pfmt -l $LABEL -s error -g $CAT:61 "options -r and -l incompatible\n"
	usage
fi
if test $# -gt 0
then
	pfmt -l $LABEL -s error -g $CAT:80 "arguments not supported\n"
	usage
fi
if test \( "$FSType" = "rfs" -o "$FSType" = "nfs" -o "$FSType" = "nucfs" \) -a "$LFLAG" = "l"
then
	pfmt -l $LABEL -s error -g $CAT:64 "option -l and FSType are incompatible\n"
	usage
fi
if test \( "$FSType" = "vxfs" -o "$FSType" = "s5" -o "$FSType" = "ufs" -o "$FSType" = "bfs" -o "$FSType" = "sfs" \) -a "$RFLAG" = "r"
then
	pfmt -l $LABEL -s error -g $CAT:65 "option -r and FSType are incompatible\n"
	usage
fi

# Undo mounts in reverse order.
awk '{ line[i++] = $0; } END { while (i > 0) print line[--i]; }' </etc/mnttab |

(
	PENDING_UMOUNT=

	# Make sure that /usr is umounted last, because we need /usr/bin/fuser.
	USRDEV=
	USRTYPE=

	umount_one()
	{
		if [ -n "$FSType" -a "$FSType" != $1 ]
		then
			return
		fi
		if [ "$LFLAG" = "l" -a \
		     \( $1 = "rfs" -o $1 = "nfs" -o $1 = "nucfs" \) ]
		then
			return
		fi
		if [ "$RFLAG" = "r" -a $1 != "rfs" -a $1 != "nfs" -a \
		     $1 != "nucfs" ]
		then
			return
		fi
		if [ ${kill} ]
		then
			/usr/sbin/fuser -k $2 >/dev/null 2>&1
			PENDING_UMOUNT="${PENDING_UMOUNT} $2"
		else
			/sbin/umount $2
		fi
	}

	while read dev mountp fstype rest
	do
		case "${mountp}" in
		/usr )
			USRDEV=$dev
			USRTYPE=$fstype
			;;
		/  | /stand | /proc | /dev/fd | '' )
			continue
			;;
		* )
			umount_one $fstype $dev
			;;	
		esac
	done

	if [ -n "$USRDEV" ]
	then
		umount_one $USRTYPE $USRDEV
	fi

	if [ "${PENDING_UMOUNT}" != "" ]
	then
		# allow time for kills to finish from /usr/sbin/fuser
		sleep 10
		for dev in ${PENDING_UMOUNT}
		do
			/sbin/umount ${dev}
		done
	fi
)
