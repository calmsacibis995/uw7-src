#ident	"@(#)pkgmrgconf.sh	15.1"
#ident	"$Header$"

# rm tmp files and exit $1

cleanup_and_exit ()
{

	[ "$UPDEBUG" = "YES" ] && {
		set -x
		goany "exiting $0"
	}

	rm -f /tmp/response.$$
	exit $1
}

nomrgmsg()
{

	[ "$UPDEBUG" = "YES" ] && set -x && goany "nomrgmsg"

	#
	# Send email to root about the location of preserved files
	# for the package $NAME. The message is in $UPGRADE_MSGS/mergefiles.1
	#

	echo "$NAME:\n" >/tmp/$$.nomrg

	cat /tmp/$$.nomrg $UPGRADE_MSGS/mergefiles.1 >/tmp/$$.nomrgmsg
	echo >>/tmp/$$.nomrgmsg

	STRING=`pfmt -s nostd -g uxupgrade:7 "System Files Preserved for %s" $PKGINST 2>&1`
	mailx -s "$STRING" root $LOGNAME </tmp/$$.nomrgmsg

	# write 'no mrg' msg in /var/sadm/install/logs/$PKGINST.log

	cat /tmp/$$.nomrgmsg >>$PKGLOG

	rm /tmp/$$.nomrgmsg /tmp/$$.nomrg

	# If installing a set, $SETINST must be set and its set.6 must be there.
	#
	# We'll display the list of pkgs for which files were not merged
	# and were preserved in /var/sadm/upgrade. The message is in
	# $UPGRADE_MSGS/mergefiles.4
	# <set>/postinstall and <set>/set.6 rely on info being set here

        [ "$SETINST" ] && SET_INSTALL=$MENU_DIR/$SETINST/set.6

	[ -f "$SET_INSTALL" ] && {

		# $UPGRADE_STORE/nomrg.list contains: "pkg1  pkg2  ..."

		[ -f $UPGRADE_STORE/nomrg.list ] && {

			read NOMRGLIST < $UPGRADE_STORE/nomrg.list

			# append 2 spaces at the end of the list.

			NOMRGLIST="$NOMRGLIST  "
		}

		NOMRGLIST="${NOMRGLIST}${PKGINST}"

		echo "$NOMRGLIST" >$UPGRADE_STORE/nomrg.list

	}

}

mrgfailmsg()
{

	[ "$UPDEBUG" = "YES" ] && set -x

	NAME="$1"	#This is the full package name

	FAILLIST="$2"	#list of files whose merge failed

	export FAILLIST NAME

	[ "$UPDEBUG" = "YES" ] && goany "mrgfailmsg"

	#
	# Send email to root about the combine failure
	# for the package $NAME.  Append the merge failed list to
	# to the msg in  $UPGRADE_MSGS/mergefiles.2
	#

	echo "$NAME:\n" >/tmp/$$.mrgfail.1

	cat /tmp/$$.mrgfail.1 $UPGRADE_MSGS/mergefiles.2 >/tmp/$$.mrgfail.2

	echo >>/tmp/$$.mrgfail.2

	cat /tmp/$$.mrgfail.2 $FAILLIST >/tmp/$$.mrgfail.1

	STRING=`pfmt -s nostd -g uxupgrade:8 "File Merge Failure for %s" $PKGINST 2>&1`
	mailx -s "$STRING" root $LOGNAME \
		</tmp/$$.mrgfail.1

	# write 'mrg fail' msg to /var/sadm/install/logs/$PKGINST.log

	cat /tmp/$$.mrgfail.1 >>$PKGLOG

	# If installing a set, $SETINST must be set and its set.6 must be there.
	#
	# We'll display the list of pkgs for which merge failed.
	# The merge failed message is in $UPGRADE_MSGS/mergefiles.2

	# <set>/postinstall and <set>/set.6 rely on info being set here

        [ "$SETINST" ] && SET_INSTALL=$MENU_DIR/$SETINST/set.6

	[ -f "$SET_INSTALL" ] && {

		# $UPGRADE_STORE/mrgfail.list contains: "pkg1  pkg2  ..."

		[ -f $UPGRADE_STORE/mrgfail.list ] && {

			read MRGFAILLIST < $UPGRADE_STORE/mrgfail.list

			# append 2 spaces at the end of the list.

			MRGFAILLIST="${MRGFAILLIST}  "
		}

		echo "${MRGFAILLIST}${PKGINST}" >$UPGRADE_STORE/mrgfail.list

		# $UPGRADE_STORE/mrgfail.files is the list of specific
		# files for various pkgs for which merge failed

		[ -f $UPGRADE_STORE/mrgfail.files ] && echo >> $UPGRADE_STORE/mrgfail.files

		echo "$NAME:" >/tmp/$$.mrgfail.1

		cat /tmp/$$.mrgfail.1 $FAILLIST >/tmp/$$.mrgfail.2

		cat /tmp/$$.mrgfail.2 >> $UPGRADE_STORE/mrgfail.files

	}

	# clean up

	rm /tmp/$$.mrgfail.1 /tmp/$$.mrgfail.2

	[ "$UPDEBUG" = "YES" ] && goany " exiting mrgfailmsg"
}

#main()

#	This script is called from the postinstall script
#	It has three args.
#	The 1st arg is $PKGINST.
#	The second is $AUTOMERGE, which is either yes or no
#	The third is $NAME,  the full package name

#	If AUTOMERGE=Yes, ${SCRIPTS}/pkgmrgconf.sh will 
#	merge the config files listed in $UPGRADE_STORE/${PKGINST}.sav.
#	If merge failed, it informs user which files the merge failed.


#	If AUTOMERGE=No, ${SCRIPTS}/pkgmrgconf.sh will 
#	inform user where there old config files live and that
#	the system will use new versions of the config. files
#	Exit code are:
#	0	success
#	1	merge failure
#	10	args not equal to 3
#	11	AUTOMERGE is neither Yes nor No
#	12	PKGINSTALL_TYPE is null or NEWINSTALL

#  must have 3 args

[ $# = 3 ] || {
	pfmt -s nostd -g uxupgrade:2 "usage: %s %s %s %s\n" $0 \${PKGINST} \${AUTOMERGE} \${NAME} 2>&1
	exit 10
}

UPGRADE_MSGS=$MENU_DIR/upgrade

SCRIPTS=/usr/sbin/pkginst

. $SCRIPTS/updebug

[ "$UPDEBUG" = "YES" ] && set -x

SPACE=" "

Set_LANG
Chk_Color_Console
export TERM

#UPGRADE_STORE=/var/sadm/upgrade
UPGRADE_STORE=/etc/inst/save.user
UP_ORIG=/etc/inst/save.orig
UPGFILE=$UPGRADE_STORE/${PKGINST}.env
sav=$UPGRADE_STORE/$1.sav
REJDIR=$UPGRADE_STORE/rejdir
mergelist=/etc/inst/up/patch/$1.LIST
#
#actually the merge failed files are stored in $mergelist.rej
#not in mergefailes...need some work here...i will set mergefailed to
#be based on mergelist....
#mergefailed=$REJDIR/$1.rej
mergefailed=${mergelist}.rej

PKGLOG=/var/sadm/install/logs/$PKGINST.log

pkg=$1
AUTOMERGE=$2
NAME="$3" 	#  $NAME contains the name of the package

# if running in auto install mode, read in the vars in $UPGFILE
# created by pkgsavfiles.sh

[ -s "$UPGFILE" ] && . $UPGFILE
rm -f $UPGFILE

[ "$UPDEBUG" = "YES" ] && goany

case "$AUTOMERGE" in

   No)  # do the 'no merge' msg
	nomrgmsg "$NAME"
	cleanup_and_exit 0
	;;

   Yes)	case "$PKGINSTALL_TYPE" in
	   UPGRADE | UPGRADE2 )
		# merge config files listed in $sav
		# pathnames in $sav are relative as up_merge expects relative 
		# path. up_merge creates the list of files for which merge 
		# failed in "$mergefailed"

		# up_merge appends '.rej' to the path passed as arg to it.
		# $UPGRADE_STORE/${PKGINST}.sav may end up in a filename
		# longer than 14 chars when '.rej' is appended to it.
		# Therefore we cp $UPGRADE_STORE to $REJDIR/${PKGINST}
	
		[ -d "$REJDIR" ] || mkdir -p $REJDIR
# i commented this out because ORIG list does not have the options..
		cp ${UP_ORIG}/${1}.LIST $mergelist

		rm -f "$mergefailed"

		[ "$UPDEBUG" = "YES" ] && goany

		${SCRIPTS}/up_merge $mergelist
		rc=$?

		[ $rc = 0 ] || {	#merge was not successful

			[ -f "$mergefailed" ] || {

		  		echo "up_merge did not create $mergefailed" \
							>"$mergefailed"
			}

			[ "$UPDEBUG" = "YES" ] && goany

			mrgfailmsg "$NAME" "$mergefailed"
		}

		rm -f $mergelist

		[ "$UPDEBUG" = "YES" ] && goany

		cleanup_and_exit $rc
		;;

	   OVERLAY)

		cd $UPGRADE_STORE

		cat $sav | grep -v '^[ 	]*#' |
			grep -v '^[ 	]*$' |
			awk '{print $1}' |
			cpio -pdmu /	>>$UPERR 2>&1

		# clean up restored files

		cat $sav | xargs rm -f 

		find . -type d -depth -print | xargs rmdir 1>/dev/null 2>&1

		rm -f $sav

		[ "$UPDEBUG" = "YES" ] && goany

		cleanup_and_exit 0
	   	;;

	   *)	exit 12    #PKGINSTALL_TYPE is neither UPGRADE, UPGRADE2 nor OVERLAY

	esac ;;

   *)	exit 11 ;;	#AUTOMERGE is neither Yes nor No

esac
