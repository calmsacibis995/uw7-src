#ident	"@(#)proto:desktop/buildscripts/sizer.sh	1.2"
#
# Script to be used in conjunction with 'sizer.awk' script to add
# sizes of binaries from the pkgmap files of the packages in the
# UnixWare set, given the path to the package images.
#
# Some manual alteration of the output data is needed, for example
# the UWdocs package has a variable in the directory, so the files
# get counted in the root filesystem, whereas the default installation
# place is in the '/usr' filesystem.
#
# Also some packages have 'space' files that mention additional space
# requirements above and beyond the files called for in the pkgmap
# file, those needs have to be added in by hand.

# The output of this script is what you see at the top of the 'size_chk'
# function.

[ $# -ne 1 -o ! -d "$1" ] && {
	echo "Usage: $0 <dir>"
	echo "	Where '<dir>' is the directory containing the package images"
	exit 1
}
[ -s $1/UnixWare/setinfo ] || {
	echo $0: Error: Cannot find $1/UnixWare/setinfo >&2
	exit 1
}
while read i rest 
do
	[ -f $1/$i/pkgmap ] &&
		awk -f $PROTO/desktop/buildscripts/sizer.awk $1/$i/pkgmap
done <$1/UnixWare/setinfo > /tmp/UnixWare.sizes$$
cat /tmp/UnixWare.sizes$$ | sort -u
rm /tmp/UnixWare.sizes$$ 
exit 0
