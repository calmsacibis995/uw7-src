#ident	"@(#)pkgsavfiles.sh	15.1"
#ident	"$Header$"

# this routine is done if auto install mode was selected
# and PKGINSTALL_TYPE for this pkg is not known

find_install_type ()
{
	[ "$UPDEBUG" = "YES" ] && set -x

	AUTOMERGE=Yes

	PKGINSTALL_TYPE=NEWINSTALL

#chkpkgrel, returns a code, indicating which version of this pkg is installed.
#Return code 2 indicates overlay of the same or older version. For overlay,
#existence of the file $UPGRADE_STORE/$PKGINST.ver indicates presence of older
#version. This file contains the old version.

#	${SCRIPTS}/chkpkgrel returns    0 if pkg is not installed
#					1 if pkg if unknown version
#					2 if OVERLAY
#					4 if UPGRADE  (from uw1.1.x)
#					6 if UPGRADE2 (from sbird2.0.1)
#					9 if newer pkg is installed
	
	[ "$UPDEBUG" = "YES" ] && goany "running $SCRIPTS/chkpkgrel from $0"
	${SCRIPTS}/chkpkgrel
	PKGVERSION=$?
	case $PKGVERSION in
		2)	PKGINSTALL_TYPE=OVERLAY	;;
		4)	PKGINSTALL_TYPE=UPGRADE	;;
		6)	PKGINSTALL_TYPE=UPGRADE2	;;
		9)	exit 3	;; 		# pkgrm newer pkg before 
						# older pkg can be installed.
		*)	AUTOMERGE=NULL ;;
	esac

	# we are in this procedure in case of auto install, when request
	# script is bypassed.
	# rm $UPGFILE so that the env vars get written into $UPGFILE
	# in write_vars_and_exit.

	rm -f $UPGFILE

	[ "$UPDEBUG" = "YES" ] && goany
}

# arg 1 for this routine is either 0 or 1
# this routine sets AUTOMERGE to NULL if arg1 is 1, saves vars and exits with $1

write_vars_and_exit () {

	[ "$UPDEBUG" = "YES" ] && set -x

	[ "$1" = 1 ] && AUTOMERGE=NULL

	# if $UPGFILE exists, the environment has AUTOMERGE and
	# PKGINSTALL_TYPE defined already because the request 
	# script must have been run

	[ -f "$UPGFILE"  ] && { 	# contains exit code of chkpkgrel
		# reset AUTOMERGE to NULL
		[ $1 = 1 ] && echo "AUTOMERGE=\"$AUTOMERGE\"" >$UPGFILE

		[ "$UPDEBUG" = "YES" ] && goany "exiting $0"
		exit $1
	}

	echo "AUTOMERGE=\"$AUTOMERGE\"" >$UPGFILE
	echo "PKGINSTALL_TYPE=\"$PKGINSTALL_TYPE\"" >>$UPGFILE
	[ "$UPDEBUG" = "YES" ] && goany "exiting $0"
	exit $1
}

### main()

#  This script is called with 1 arg, $PKGINST, from the preinstall script

#  For upgrade the volatile files list is:
#	/etc/inst/up/patch/${PKGINST}.LIST
#
#  For overlay, this script gets the volatile files from the
#  	'contents' file for  $PKGINST
#  It saves the 'v' files in /etc/inst/save.user
#  It makes a list of the files saved in  in ${UPGRADE_STORE}/$1.sav

#  Exit codes are:
#   0 	when volatiles are saved in $UPGRADE_STORE
#   1 	no volatile files exist for $PKGINST so AUTOMERGE is set to NULL

SCRIPTS=/usr/sbin/pkginst

. $SCRIPTS/updebug

[ "$UPDEBUG" = "YES" ] && set -x


[ "$1" ] || {
	pfmt -s nostd -g uxupgrade:3 "Usage: %s <PKGINST>\n" $0 2>&1
	echo "Usage: $0  <PKGINST>"  >>$UPERR
	exit 2
}
	
pkg=$1

S=" ";			# space
T="	";		# tab
UPGLIST=/etc/inst/up/patch/${1}.LIST
UPGRADE_STORE=/etc/inst/save.user
[ -d $UPGRADE_STORE ] || mkdir -p $UPGRADE_STORE
UPDIR=/etc/inst/up
[ -d $UPDIR ] || mkdir -p $UPDIR

sav=$UPGRADE_STORE/$1.sav
rm -f $sav

UPGFILE=$UPGRADE_STORE/${1}.env

# $UPGFILE is created if pkgchkrel has been run from the request script.
# remember that request script is bypassed in AUTOMATIC install mode

[ "$UPDEBUG" = "YES" ] && goany

[ -f "$UPGFILE" ] || find_install_type

# PKGINSTALL_TYPE must  be set by now

[ "$PKGINSTALL_TYPE" ] || { 
	pfmt -s nostd -g uxupgrade:4 "Error: %s:   PKGINSTALL_TYPE must be set by now\n" $0 2>&1
	echo "Error: $0:   PKGINSTALL_TYPE must be set by now"  >>$UPERR
	exit 2
}

[ "$PKGINSTALL_TYPE" = NEWINSTALL ] && write_vars_and_exit 1

[ "$PKGINSTALL_TYPE" = UPGRADE2 -o "$PKGINSTALL_TYPE" = UPGRADE ] && {

	[ -f "$UPGLIST" ] || {
		echo "Error: $0:   $UPGLIST missing"  >>$UPERR
		write_vars_and_exit 1
	}

	# eliminate comment lines from the list of volatile files
	cat $UPGLIST | grep -v '^[ 	]*#' | grep -v '^[ 	]*$' >$sav

	[ -s $sav ] || {
		echo "Error: $0:   $UPGLIST has 0 non-commentary lines"  >>$UPERR
		write_vars_and_exit 1
	}
}

[ "$UPDEBUG" = "YES" ] && goany

[ "$PKGINSTALL_TYPE" = OVERLAY ] && {

	CONTENTS=/var/sadm/install/contents
	olist=/tmp/list.$$

	# grep volatile files from 'contents'  and save temporarily in $sav

	# first grep for $pkg in the end of line

	PATTERN="${S}${T}+-~*\:!"
	grep "^.*[${T}${S}][ve][${T}${S}].*[$PATTERN]${pkg}$" $CONTENTS  \
						 >$sav   2>>$UPERR
	# now grep for $pkg preceeded and followed by any of the chars:
	# whitespace  * \ ~ : ! + -

	grep "^.*[${T}${S}][ve][${T}${S}].*[${PATTERN}]${pkg}[${PATTERN}]" \
			$CONTENTS >>$sav   2>>$UPERR
	
	# get rid from the list, the files in /etc/conf that are 
	# marked 'v' in $CONTENTS

	grep -v "^/etc/conf/" $sav | grep -v "^/tmp/" >$olist 2>>$UPERR
	grep  "^/etc/conf/init.d/kernel" $sav >>$olist 2>>$UPERR
	grep  "^/etc/conf/node.d/" $sav >>$olist 2>>$UPERR
	grep  "^/etc/conf/mtune.d/" $sav >>$olist 2>>$UPERR
	grep  "^/etc/conf/cf.d/stune" $sav >>$olist 2>>$UPERR

	#if empty list, exit 1 to set AUTOMERGE to NULL

	[ -s $olist ] || {

		# We don't want $sav to be lying around. Also clean up $olist

		rm -f $sav $olist
		echo "Error: $0:   no volatile files for $PKGINST"  >>$UPERR
		write_vars_and_exit 1
	}
	[ "$UPDEBUG" = "YES" ] && goany

	#  get the file name (1st field), make it relative path, save final
	#  list of volatile files that will be preserved in $sav

	cat $olist | sed 's/[ 	].*$//' | sed 's/^\///'  >$sav
	rm -f $olist
}

[ "$UPDEBUG" = "YES" ] && goany

# save files listed in $sav in $UPGRADE_STORE
#

cd /
cat $sav | cpio -pdmu $UPGRADE_STORE 	  >>$UPERR 2>&1

[ "$UPDEBUG" = "YES" ] && goany

write_vars_and_exit 0
