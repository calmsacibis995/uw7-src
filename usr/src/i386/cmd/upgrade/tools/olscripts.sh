#ident	"@(#)olscripts.sh	15.1"

SCRIPTS=/usr/sbin/pkginst
. $SCRIPTS/updebug

[ "$UPDEBUG" = "YES" ] && set -x

Set_LANG
Chk_Color_Console
export TERM

[ "$1" = "1" ]  && {
	### main ()
	#  interim script to display the screen:
	#  
	#  Do you want to auto merge config files or not
	#
	#  this will not be needed once the new menu tool is in place
	#  The 1st arg is the name of the pkg
	#  This script is invoked from  the request script of a pkg
	
	NAME="$2"

#
# First, we need to create the menu program 
############	
cat >/tmp/menufile.xx << !
.optstring
Your choices are:
.pageno
Page %d of %d
.ul
UNIX System `echo "${NAME}"` 
.hhelp_ban
Help on Help and How to Use the Menu Tool
.helpbanner
Help on Removing Old Packages
.ur
Package Removal
.ll
.lr
Del=Cancel 
.top
!
echo >>/tmp/menufile.xx
echo "The following packages are installed:" >> /tmp/menufile.xx
echo "" >> /tmp/menufile.xx
[ -s "/tmp/oleus.xx" ] && cat /tmp/oleus.xx  >>/tmp/menufile.xx
[ -s "/tmp/olxt.xx" ] && cat /tmp/olxt.xx  >>/tmp/menufile.xx
echo "" >>/tmp/menufile.xx
cat >> /tmp/menufile.xx << !
.form
Remove the existing packages.  Install the new packages.
Do not remove the existing packages.  New versions will not be installed.
.selection
Press '1' or '2' followed by ENTER/CONTINUE:
.help
No help for this step.
.helpinst
Del=Cancel  F1=Help  ESC=Exit Help  
.working
Working...
.end
!
############	
	#
	#  Now invoke the menu program with everything we just extracted.
	#
		unset RETURN_VALUE
		menu  -f /tmp/menufile.xx -o /tmp/response </dev/tty
		. /tmp/response
		rm -f /tmp/response
		
	#	returns 1 for Yes. Remove the packages.
	#	returns 2 for No. Do not remove packages.
	rc=`expr $RETURN_VALUE - 1`
	unset RETURN_VALUE
	rm -f /tmp/menufile.xx
	exit $rc
}
[ "$1" = "2" ] && {
	CONTENTS=/var/sadm/install/contents
	GREPFILE=/tmp/pkgrem.$$
	PACKAGE=$2
	FILELIST=$3
	TMPLIST=/tmp/tmpfiles.$$
	echo >$TMPFILE
	echo >$GREPFILE
	echo >$FILELIST
	grep " $PACKAGE " $CONTENTS >> $GREPFILE
	grep " $PACKAGE$" $CONTENTS >> $GREPFILE
	cat $GREPFILE | cut -f1 -d" " >> $TMPLIST
	/usr/bin/sort -r $TMPLIST > $FILELIST
	if [ -s $FILELIST ]
	then
	while read filename
	do
		if [ -f "$filename" ]
		then
			rm -f $filename
		fi
		if [ -d "$filename" ]
		then
			rmdir $filename
		fi
	done < $FILELIST
	fi
	rm -f $GREPFILE $TMPLIST
	export FILELIST PACKAGE
}
[ "$1" = "3" ] && {
	#
	#  This script is invoked from the request script of a package
	#
	#  It checks if the pkg being installed exists on the system
	#  If so, it checks the VERSION of the INSTALLED pkg.
	#  It  exits with code:
	#	6, if V4 and no olxt/oleus packages
	#	5, if V4 and olxt/oleus packages
	#	4, if DESTINY and oleus/olxt packages
	#       2, if DESTiny and no oleus/olxt packages
	#       1, the installed pkg is any other ver
	#       0, if pkg not installed or  other problems
	#
	#
	export PKGVERSION 
	PKGVERSION=0		#pkg in not installed
	[ "$2" ] && {
	OLEUS=`/usr/bin/pkginfo  oleus 2>/dev/null`
	[ "$OLEUS" ] && echo "$OLEUS" > /tmp/oleus.xx
	OLXT=`/usr/bin/pkginfo olxt 2>/dev/null`
	[ "$OLXT" ] && echo "$OLXT" > /tmp/olxt.xx
	[ "$OLEUS" ] || [ "$OLXT" ] && exit 1
	exit 0
	}
	PKGNAME=$PKGINST
	VERSION=`/sbin/uname -v`
	PACKAGE=`/usr/bin/pkginfo $PKGNAME 2>/dev/null`
	[ "$VERSION" = "4" ] && exit 4
	[ "$VERSION" = "V4ES" ] || exit 1
	[ "$PACKAGE" ] || exit 0
	exit 2
}
[ "$1" = "4" ] && {
	
	#   this script is called from the postinstall script of a pkg
	
	nomrgscreen()
	{
	NAME="$2"		#This is the full package name
	export NAME

	menu -f ${UPGRADE_MSGS}/mergefiles.1 -o /tmp/response </dev/tty
	rm -f /tmp/response
	}
	
	mrgfailscreen()
	{
	NAME="$2"		#This is the full package name
	FAILLIST=$3	#list of files whose merge failed
	export NAME FAILLIST
	#
	#  Now invoke the menu program with everything we just extracted.
	#
		menu  -f ${UPGRADE_MSGS}/mergefiles.2 -o /tmp/response </dev/tty
	}
	
	#main()
	
	#	This script is called from the postinstall script
	#	It has three args.
	#	The 1st arg is the pkg (abbrev.) for which it is called
	#	The second is $AUTOMERGE, which is either yes or no
	#	The third is $NAME,  the full package name
	
	#	If AUTOMERGE=Yes, ${SCRIPTS}/pkgmrgconf.sh will 
	#	merge the config files listed in $UPGRADE_STORE/${PKGINST}.sav.
	#	If merge failed, it informs user which files the merge failed.
	
	
	#	If AUTOMERGE=No, ${SCRIPTS}/pkgmrgconf.sh will 
	#	inform user where there old config files live and that
	#	the system will use new versions of the config. files
	
	#  must have 3 args
	[ $# = 4 ] || {
		echo "usage: $0 \${PKGINST} \${AUTOMERGE} \${NAME}" >>$UPERR
		exit 1
	}
	UPDIR=/etc/inst/up
	UPERR=$UPDIR/${1}.out
	[ -d $UPDIR ] || mkdir -p $UPDIR
	
	SPACE=" "
	
#	UPGRADE_STORE=/var/sadm/upgrade
	UPGRADE_STORE=/etc/inst/save.user
	[ -d $UPGRADE_STORE ] || mkdir -p $UPGRADE_STORE
	list=$UPGRADE_STORE/$2.sav
	faillist=$UPGRADE_STORE/$2.rej
	
	pkg=$2
	AUTOMERGE=$3
	NAME="$4"
	
	#  $NAME contains the name of the package
	
	[ "$AUTOMERGE" = No ] && {
	
		#  do the 'no merge' screen
		nomrgscreen "$NAME"
		exit 0
	}
	
	[ "$AUTOMERGE" = Yes ] && {
	
		#	merge config files listed in $list
		rm -f $faillist
		# upgrade_merge expects relative pathname
		# pathnames in $list are relative
		# upgrade_merge  creates the list of files
		# for which merge failed in $faillist
	
		[ "$PKGINSTALL_TYPE" = "UPGRADE" ] && {
		echo ""
		echo ""
		pfmt -s nostd -g uxupgrade:1 "The following files could not be merged. They have been saved in /var/sadm/upgrade:\n"
		echo ""
		echo "		/usr/X/defaults/Xwinconfig"
		echo ""
		[ -s /var/sadm/upgrade/usr/X/defaults/Xwincmaps ] && {
		cp /var/sadm/upgrade/usr/X/defaults/Xwincmaps /usr/X/defaults
		}
		rc=$?
		[ $rc = 0 ] && exit 0	#merge was successful
		}
		[ "$PKGINSTALL_TYPE" = "OVERLAY" ] && {
		echo ""
		echo ""
		pfmt -s nostd -g uxupgrade:1 "The following files could not be merged. They have been saved in /var/sadm/upgrade:\n"
		echo ""
		echo "		/usr/X/defaults/Xwinconfig"
		echo "		/usr/X/defaults/Xwinconfig.ini"
		echo "		/usr/X/defaults/Xwinfont"
		echo ""
		[ -s /var/sadm/upgrade/usr/X/defaults/Xwincmaps ] && {
		cp /var/sadm/upgrade/usr/X/defaults/Xwincmaps /usr/X/defaults
		}
		rc=$?
		[ $rc = 0 ] && exit 0	#merge was successful
		}
	
		#	if merge fails
	
		#mrgfailscreen "$NAME" $faillist
		exit 1
	}
}
