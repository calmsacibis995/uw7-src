#ident	"@(#)rmwhite.sh	15.1"

function rmwhite
{
	(( $# == 2 )) || {
		print -u2 "ERROR:USAGE: $0 infile outfile"
		return 1
	}

	# this script removes all leading white space, and then
	# removes all blank lines and lines starting with "#", except if the
	# "#" is followed by a "!" as in "#!/usr/bin/ksh"
	rm -f $2 >/dev/null 2>&1 	#Make sure the file to copy to is gone,
					#in case it's not writable
	checkwhite < $1
	case $? in
	0)
		print -u2 "$0: filtering whitespace from $1"
		sed -e 's/^[ 	]*//' -e '/^#[^!]/D' -e '/^$/D' -e '/^#$/D' $1 > $2
		;;
	1)
		print -u2 "$0: WARNING: here doc in $1: filtering whitespace anyway"
		sed -e 's/^[ 	]*//' -e '/^#[^!]/D' -e '/^$/D' -e '/^#$/D' $1 > $2
		;;
	2)
		print -u2 "$0: NOT filtering whitespace from $1"
		cp $1 $2
		;;
	esac
	return 0
}
