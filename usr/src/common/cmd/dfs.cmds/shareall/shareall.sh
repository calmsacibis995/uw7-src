#!/sbin/sh
#	copyright	"%c%"

#ident	"@(#)shareall.sh	1.2"
#ident  "$Header$"
# shareall  -- share resources

LABEL="UX:shareall"
CATALOG="uxdfscmds"

USAGE="Usage: shareall [-F fsys[,fsys...]] [- | file]\n"
fsys=

while getopts F: i
do
	case $i in
	F)  fsys=$OPTARG;;
	\?) pfmt -l $LABEL -s action -g $CATALOG:11 "$USAGE" >&2; exit 1;;
	esac
done
shift `expr $OPTIND - 1`

if [ $# -gt 1 ]		# accept only one argument
then
	pfmt -l $LABEL -s action -g $CATALOG:11 "$USAGE" >&2
	exit 1
elif [ $# = 1 ]
then
	case $1 in
	-)	infile=;;	# use stdin
	*)	infile=$1;;	# use a given source file
	esac
else
	infile=/etc/dfs/dfstab	# default
fi


if [ "$fsys" ]		# for each file system ...
then
	`egrep "^[^#]*[ 	][ 	]*-F[ 	]*(\`echo $fsys|tr ',' '|'\`)" $infile|/sbin/sh`
else			# for every file system ...
	cat $infile|/sbin/sh
fi
