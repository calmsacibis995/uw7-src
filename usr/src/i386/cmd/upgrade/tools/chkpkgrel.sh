#ident	"@(#)chkpkgrel.sh	15.1"
#ident	"$Header$"

#main()
#  This script is invoked from the request script of a package, therefore
#  $PKGINST must be defined. The script is optionally called with one arg.
#  If there is an arg, then it checks the version of $1
#
#  It checks if the $PKGINST or $1 exists on the system.
#  If so, it checks its VERSION.  It  exits with code:
#	9, if $PKGINST (or $1) that is installed is a newer version 
#	6, if $PKGINST (or $1) is installed and is 2.01 version (UPGRADE2)
#	4, if $PKGINST (or $1) is installed and is 1.1 version (UPGRADE)
#	2, if $PKGINST (or $1) is installed and is current version (OVERLAY)
#			if the version of the installed package is older
#			than that being installed, we'll create a file
#			$UPGRADE_STORE/$PKGINST.ver
#       1, if $PKGINST (or $1) is neither a UPGRADE or OVERLAYable
#       0, if $PKGINST (or $1) is not installed or  other problems
#
#  This scripts is also invoked if a package is run in automatic mode.
#  It is called from preinstall script either directly, or from 'pkgsavfiles'.
#  If called in automatic mode, the menu to warn the user to remove the newer
#  version, should have been handled via menu (set.8) of the set request script.#  Therefore, no user input is needed in the preinstall stage!

SBINPKGINST=/usr/sbin/pkginst

. $SBINPKGINST/updebug

# If PKGINST is in the file /tmp/pkg.newinstall, it is being installed
# from the boot floppy, but it was not on the system originally.  So the
# value of PKGINSTALL_TYPE which might be set in the common.sh file is
# inaccurate, and this package should do a NEWINSTALL

grep "^$PKGINST$" /tmp/pkg.newinstall >/dev/null 2>&1  && exit 0

# If chkpkgrel is running from a request script run out
# of the postreboot script after a boot floppy installation,
# get the install type out of the file saved it from the
# boot floppy

[ -f /etc/inst/scripts/common.sh ] && {
	. /etc/inst/scripts/common.sh
	case $PKGINSTALL_TYPE in
		NEWINSTALL)
			exit 0
			;;
		OVERLAY)
			exit 2
			;;
		UPGRADE)
			exit 4
			;;
		UPGRADE2)
			exit 6
			;;
	esac
}
#
#  Call Set_LANG defined in updebug to make sure LANG environment variable 
#  is set.  UPGRADE_MSGS and menu_color will also be set.

[ "$UPDEBUG" = "YES" ] && set -x
Set_LANG
Chk_Color_Console
export TERM

PKGVERSION=0		#pkg is not installed

# exit 0, if the user selected destructive install
# That is, INSTALL_TYPE=NEWINSTALL in /var/sadm/upgrade/install_type
[ -f /var/sadm/upgrade/install_type ] && {
	. /var/sadm/upgrade/install_type
	[ "$INSTALL_TYPE" = NEWINSTALL ] && exit $PKGVERSION
}

pkg=$PKGINST

[ "$1" ] && pkg="$1"	#check existence/version of pkg $1

UPDIR=/etc/inst/up
[ -d $UPDIR ] || mkdir -p $UPDIR

#UPGRADE_STORE=/var/sadm/upgrade
UPGRADE_STORE=/etc/inst/save.user
[ -d $UPGRADE_STORE ] || mkdir -p $UPGRADE_STORE

PKGINFO=/var/sadm/pkg/$pkg/pkginfo

[ "$1" ] && {

	# Note that if $1 is set, we are checking for a package other than 
	# that being installed. We will set VERSION to 999. This means that 
	# if current pkg is installed, we'll always return code 2 (indicating 
	# OVERLAY). chkpkgrel is invoked with an arg when a pkg 
	# being installed wants to check if another pkg is installed and if 
	# yes, what version of it is installed. We don't want to return 9
	# in this case.

	VERSION=999

}

# if $PKGINFO does not exist, this must be a new installation or
# a version of the package which did not support 'pkginfo' is installed.
# For all these cases we exit with code 0, implying destructive installation


[ "$UPDEBUG" = "YES" ] && goany

[ -f $PKGINFO ] || exit $PKGVERSION

# Get the version number of the installed package.
verline=`grep "^VERSION=" ${PKGINFO} 2>>$UPERR`

# Save IFS and restore it later.
OIFS=$IFS; IFS="="

set $verline
version=$2

# Get pstamp of installed package to retrieve release
# PSTAMP=<REL><SPACE><DATE><SPACE> optionally followed by <SPACE><LOAD>.

stamp=`grep "^PSTAMP=" ${PKGINFO} 2>>$UPERR`

set $stamp
# $1 is PSTAMP	$2 is <REL><SPACE><DATE>, optionally followed by <SPACE><LOAD>.
# For SVR4.0 V4 $2 is not set.

[ "$2" ] && {
	IFS=" "	   # space
	set $2	   # now $1 is <REL>, $2 is <DATE>, and $3, if set, is load.
	REL=$1	   # REL (release) is set to SVR4.2
}

# for SVR4.0 V4 we'll read RELEASE.
[ "$REL" ] || {
	release=`grep "^RELEASE=" ${PKGINFO} 2>>$UPERR`	# RELEASE=4.0
	set $release
	REL=$2
}

IFS=$OIFS		# now reset IFS to the original IFS

PKGVERSION=1		# unknown version
[ "$version" ] && {
	# $VERSION is the version being installed.
	# $version is the installed version

	case "$REL" in

	    SVR4.2MP | UW2.0 | UW2.01)
		PKGVERSION=6 ;;		# UW 2.0 or 2.01 package

	    UW2.1)	#current PSTAMP as of eiger3, I suspect it will change

		[ $version -gt $VERSION ] && {	

			# If CALLED_FROM_SET is set, set request will do the
			# the warning screen to pkgrm the newer package(s).

			[ "$CALLED_FROM_SET" ] && exit 9

			# Warn user to pkgrm the newer version installed.
			# If version 1 pkg is being installed on a newer pkg,
			# we'll ask the user to hit the delete key in the
			# warning screen to terminate installation.
			# This will prevent trashing of configuration files
			# of the newer package.

			menu_colors warn
			export PKGINST NAME VERSION version 

			WARN_MSG=$UPGRADE_MSGS/rm.newerpkg
			menu -f ${WARN_MSG} -o /dev/null

			# If installing version 1 on top of a newer version, 
			# send interrupt signal to all processes in this group
			# so that installation is terminated and configuration 
			# files are not trashed.
			[ $VERSION = 1 ] &&  kill -2 0

			# If installing versions other than 1 on top of newer
			# version, exit with code 9 to inform my parent to 
			# signal installation termination to pkgadd by retuning
			# code 3 to pkgadd.
			exit 9
		}

		[ $version -eq $VERSION ] && {
			# same SVR4.2 package
			# Return code 2 for OVERLAY  only if the pkg 
			# is completely installed, 

			PKGVERSION=2 	
		}
		[ $version -lt $VERSION ] && {
			# Older SVR4.2 package. Return code will be 2 (OVERLAY).
			# If any special processing is to be done for upgrade of
			# an older SVR4.2 package, the package install scripts
			# can look for the file $UPGRADE_STORE/${pkg}.ver

			PKGVERSION=2 	

			# save older version of the pkg being installed in
			# $UPGRADE_STORE/$pkg.ver. Note that if VERSION==999
			# $PKGINST is checking if $pkg installed, and if yes,
			# what version of $pkg is installed. In this case we do
			# not need the $UPGRADE_STORE/$pkg.ver file.

			[ $VERSION -ne 999 ] && 
				echo $version >${UPGRADE_STORE}/${pkg}.ver
		}

		[ "$version" != "$VERSION" ] && {
			# versions are not identical, but might compare
			# equal due to multiple '.' (2.0 vs 2.0.1 for example)
			# Make $UPGRADE_STORE/$pkg.ver file.

			[ $VERSION -ne 999 ] &&
				echo $version >${UPGRADE_STORE}/${pkg}.ver
		} ;;

	    SVR4.2)
		PKGVERSION=4	;;	# UW 1.1
	    *)	;;
	esac

	# If /tmp/$pkg.Lock exists, the pkg is only partially installed
	# and the pkg is installed via set installation.
	# Reset the return code to 1,  for new installation of the pkg,
	# if version is checked for the pkg being installed.

	[ -f /tmp/$pkg.Lock -a "$pkg" = "$PKGINST" ] && {
		rm -f /tmp/$pkg.Lock
		PKGVERSION=1
	}
}

# Create $UPGFILE so that pkgsavfiles.sh knows not to find pkg version again
UPGFILE=$UPGRADE_STORE/$PKGINST.env

# Saving $PKGVERSION in $UPGFILE will enable to set PKGINSTALL_TYPE
# in pkgsavfiles, if it is not set already.

[ "$pkg" = "$PKGINST" ] && {
	case "$PKGVERSION" in
		6)	PKGINSTALL_TYPE=UPGRADE2
			break;;
		4)	PKGINSTALL_TYPE=UPGRADE
			break;;
		2)	PKGINSTALL_TYPE=OVERLAY
			break;;
		0)	PKGINSTALL_TYPE=NEWINSTALL
			break;;
	esac
	echo PKGINSTALL_TYPE=$PKGINSTALL_TYPE >"$UPGFILE"
}

[ "$UPDEBUG" = "YES" ] && goany

exit $PKGVERSION
