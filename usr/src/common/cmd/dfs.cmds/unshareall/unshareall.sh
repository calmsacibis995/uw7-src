#!/sbin/sh
#	copyright	"%c%"

#ident	"@(#)unshareall.sh	1.2"
#ident  "$Header$"
# unshareall  -- unshare resources

LABEL="UX:unshareall"
CATALOG="uxdfscmds"

USAGE="Usage: unshareall [-F fsys[,fsys...]]\n"
fsys=

while getopts F: i
do
	case $i in
	F)  fsys=$OPTARG;;
	\?) pfmt -l $LABEL -s action -g $CATALOG:12 "$USAGE" >&2; exit 1;;
	esac
done
shift `expr $OPTIND - 1`

if [ $# -gt 0 ]		# accept no arguments
then
	pfmt -l $LABEL -s action -g $CATALOG:12 "$USAGE" >&2
	exit 1
fi

if [ "$fsys" ]		# for each file system ...
then
	fsys=`echo $fsys|tr ',' ' '`
else			# for every file system ...
	fsys=`sed 's/^\([^# 	]*\).*/\1/' /etc/dfs/fstypes`
fi

for i in $fsys
do
	for path in `sed -n "s/^\([^ 	]*\)[ 	]*[^ 	]*[ 	]*${i}.*/\1/p" /etc/dfs/sharetab`
	do
		/usr/sbin/unshare -F $i $path
	done
done
