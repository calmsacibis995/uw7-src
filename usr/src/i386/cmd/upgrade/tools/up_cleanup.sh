#ident	"@(#)up_cleanup.sh	15.1"
#ident	"$Header$"

# If this is a new installation, and we're installing from inst and pkginst
# rather than from /usr/sbin/pkgadd, then exit now.

if [ \( "$PKGINSTALL_TYPE" = NEWINSTALL \) -a \( -f /tmp/boot.96587 \) ] 
then
	exit 0
fi

upgrade_cleanup()
{
#	UPGRADE_STORE=/var/sadm/upgrade
	UPGRADE_STORE=/etc/inst/save.user

	#
	# This is a generic procedure to clean up the extraneous patch
	# files used during an upgrade installation.  This procedure
	# should be called by every package that contains patch files.
	# It should be called regardless of type of installation:
	# NEWINSTALL, OVERLAY, or UPGRADE.
	#
	# Usage: /usr/sbin/pkginst/upgrade_cleanup $PKGINST
	#
	# Error conditions include:
	#
	#	No argument: for debugging we'll print a usage message
	#	No files in file list
	#
	# None of these errors should occur, if they do, we'll still
	# return 0, since there is really nothing we can do about it.
	#

	[ "$UPDEBUG" = YES ] && set -x

	PKG=${1}

	PATCH_LOC=/etc/inst/up/patch

	#
	# Look for everything in the patch directory that belongs to
	# this package.  The sort -r is being done so that files in
	# directories are deleted before we try to delete the directory.
	#
	# The directory should always be empty by the time we try to
	# remove it, since they are only used for patch files and each
	# package should clean up after itself.  But, we want to be safe.
	#
	# Since all the *.LIST files are part of the base, when we call
	# this from the base pkg postinstall script, we'd remove all the
	# *.LIST files for the packages that need them later, so we'll
	# explictely grep -v them out of the list.
	#

	grep "^/etc/inst/up/patch.* $PKG" /var/sadm/install/contents |
		cut -d" " -f1  | grep -v "LIST$" | sort -r >/tmp/$$.rm

	for i in `cat /tmp/$$.rm`
	do
		[ -f $i ] && rm -f $i
		[ -d $i ] && rmdir $i >/dev/null 2>&1

		removef $PKG $i >/dev/null 2>&1

		[ "$UPDEBUG" = YES ] && goany
	done

	removef -f $PKG >/dev/null 2>&1
	rm -f /tmp/$$.rm

	[ -f $PATCH_LOC/$PKG.LIST ] && {
	
		rm -f $PATCH_LOC/$PKG.LIST

		# Currently the *.LIST files are part of the base

		removef base $PATCH_LOC/$PKG.LIST >/dev/null 2>&1
		removef -f base >/dev/null 2>&1
	}

	#
	# Now clean up directories left empty.  We're doing this just
	# in case all the relevent directories are not be listed in
	# the contents file.
	#

	[ -d "$PATCH_LOC" ] &&
		find $PATCH_LOC -type d -depth -print | xargs rmdir >/dev/null 2>&1

	[ ! -s  $UPDIR/up.err ] && rm $UPDIR/up.err
	rmdir $UPDIR >/dev/null 2>&1

	rm -f ${UPGRADE_STORE}/${PKG}.env ${UPGRADE_STORE}/${PKG}.ver

	[ "$UPDEBUG" = YES ] && goany
}

# Move_WarnMsg_To_UPERR  - moves harmless warning messages from the
# package log files created by pkgadd to upnoover log file
# /etc/inst/up/up.err. This makes contents of the package log file
# not so overwhelming.
# First argument is the package whose log is to be cleaned up.
# Second argument is the warning message which is to be relocated
# in the upnover log file.

Move_WarnMsg_To_UPERR ()
{
	PKG=$1
	MSG=$2
	[ "$PKG" ] || {
		echo "up_cleanup called without arguments" >>$UPERR
		return
	}
	PKGLOG=/var/sadm/install/logs/$PKG.log
	[ -f $PKGLOG ] || {
		echo "up_cleanup called for nonexistent pkg" >>$UPERR
		return
	}
	[ -s $PKGLOG ] || {
		echo "LOG file <$PKGLOG> is empty" >>$UPERR
		return
	}
	[ "$MSG" ] || {
		echo "up_cleanup called without 2nd argument" >>$UPERR
		return
	}
	grep  "$MSG" $PKGLOG >/tmp/$$.msgs
	[ -s /tmp/$$.msgs ] && {
		grep -v "$MSG" $PKGLOG >/tmp/$$.nlog
		mv /tmp/$$.nlog $PKGLOG
		echo "Beginning of messages moved from <$PKGLOG>: " >>$UPERR
		cat /tmp/$$.msgs >>$UPERR
		echo "End of messages moved from <$PKGLOG>: " >>$UPERR
	}
	rm -rf /tmp/$$.msgs /tmp/$$.nlog
}

#Main

PKG=${1}

[ ! "$PKG" ] && {

	pfmt -s nostd -g uxupgrade:5 "Usage: %s pkginst\n" $0 2>&1
	exit 0
}

UPDIR=/etc/inst/up
SCRIPTS=/usr/sbin/pkginst
. $SCRIPTS/updebug

[ "$UPDEBUG" = YES ] && set -x

upgrade_cleanup $PKG

# Move WARNING messages about 'no longer a file'  or ' no longer a linked file'
# or 'no longer a directory' or 'no longer a symbolic link' from <pkg>.log
# to upnover log file.

Move_WarnMsg_To_UPERR  $PKG "WARNING: .*no longer "

exit 0
