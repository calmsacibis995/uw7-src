#!/sbin/sh
#	copyright	"%c%"
#ident	"@(#)fstyp:common/cmd/fstyp/fstyp.sh	1.4.7.11"
#
#	Determine the fs identifier of a file system.
#

# disambiguate SPEC TYPE1 TYPE2 [... TYPE N], putting fstype in FSTYP.
# In practice this function can deal only with disambiguating the AFS/s5
# ambiguity caused by the symlink /etc/fs/AFS -> /etc/fs/s5.
disambiguate()
{
	[ $# -ne 3 ] && return 1
	case "$*" in
	*AFS* )
		case "$*" in
		*s5*)
			FSTYP=s5
			return 0
			;;
		*S5*)
			FSTYP=S5
			return 0
			;;
		esac
	esac
	return 1
}

USAGE="Usage: fstyp [-v] special"
unset VFLAG
unset FSTYP
if [ $# -eq 0 ]
then
	echo "$USAGE" >&2
	exit 2
fi

if [ -d /usr/lib/fs ]
then
	DIR=/usr/lib/fs
else
	DIR=/etc/fs
fi

while getopts v? c
do
	case $c in
	 v) VFLAG="-"$c;;
	\?) echo "$USAGE" >&2
	    exit 2;;
	esac
done
shift `expr $OPTIND - 1`

# The following contains support for an undocumented second argument,
# a hint fs identifier, which is an attempt to speed up this command.

case $# in
1 )
	;;
2 )
	FSTYP=$2
	;;
* )
	echo "$USAGE" >&2
	exit 2
	;;
esac

SPEC=$1
if [ ! -r $SPEC ]
then
	echo "fstyp: cannot stat or open $SPEC" >&2
	exit 1
fi
if [ \( ! -b $SPEC \) -a \( ! -c $SPEC \) ]
then
	echo "fstyp: $SPEC not block or character special device" >&2
	exit 1
fi

exec 3>&2 2>/dev/null

# If we might have an fs identifier cached, try it first.
if [ -n "$FSTYP" ]
then
	# If vflag is not set, avoid creating some processes.
	if [ -z "$VFLAG" ]
	then
		if $DIR/$FSTYP/fstyp $SPEC >/dev/null
		then
			[ $FSTYP = AFS ] && FSTYP=s5	# special case
			echo $FSTYP
			exit 0
		fi
	else
		RES=`$DIR/$FSTYP/fstyp $VFLAG $SPEC`
		if [ $? -eq 0 ]
		then
			echo "$RES"
			exit 0
		fi
	fi
fi

exec 2>&3

# Either we had no fs identifier cached, or it was wrong.

#	Execute all heuristic functions /etc/fs/*/fstype 
#	or /usr/lib/fs/*/fstyp and
#	return the fs identifier of the specified file system.

# Set up for the chdir by making sure the device name is an absolute
# pathname

case $SPEC in
/* )
	;;
* )
	SPEC="`pwd`/$SPEC"
	if [ $? -ne 0 ]
	then
		echo Cannot pwd >&2
		exit 1
	fi
	;;
esac

cd $DIR
if [ $? -ne 0 ]
then
	echo Cannot chdir to $DIR >&2
	exit 1
fi

unset FSTYP
for f in *
do
	if [ -x $f/fstyp ]
	then
		$f/fstyp $VFLAG $SPEC >/dev/null 2>&1
		if [ $? -eq 0 ]
		then
			FSTYP="$FSTYP $f"
		fi
	fi
done

set -- $FSTYP

if [ $? -ne 0 -o -z "$FSTYP" ]
then
	echo "Unknown_fstyp (no matches)" >&2
	exit 1
fi

case $# in
1 )
	;;
* )
	disambiguate $SPEC $*
	if [ $? -ne 0 ]
	then
		echo "Unknown_fstyp (multiple matches)" >&2
		exit 2
	fi
	;;
esac

# The verbose case is unusual, so we can burn another process.

if [ -z "$VFLAG" ]
then
	echo $FSTYP
else
	$FSTYP/fstyp $VFLAG $SPEC
	[ $? -ne 0 ] && exit 2
fi

exit 0

