#!/sbin/sh
#	copyright	"%c%"

#ident	"@(#)initpkg:i386/cmd/initpkg/ckroot.sh	1.23.1.1"

if [ -z "$LC_ALL" -a -z "$LC_MESSAGES" ]
then
	if [ -z "$LANG" ]
	then
		LNG=`defadm locale LANG 2>/dev/null`
		if [ "$?" != 0 ]
		then LANG=C
		else eval $LNG
		fi
	fi
	LC_MESSAGES=/etc/inst/locale/$LANG
	export LANG LC_MESSAGES
fi
LABEL="UX:$0"

# We're being spawned from inittab, and /dev/console isn't available yet.
# Use the 437 encoding because these messages are being 
# redirected to /dev/sysmsg.  Export this info for child procs.
CAT=uxrc_437; export CAT

# This file has those commands necessary to check the root file
# system.


rootmntp=/
rootfs=/dev/root
rrootfs=/dev/rroot
idcache=/etc/fs/.root_type
hint=
fstyp=
exec 3>&2 2>/dev/null
[ -f $idcache -a -r $idcache ] && read hint <$idcache
exec 2>&3


# Try to get the file system type for root.  If this fails,
# it will be set to null.  In that case, we hope that vfstab
# is around so that fsck will use it for the type.

# This is an attempt to avoid the overhead of execing /sbin/fstyp.
if [ -n "$hint" -a -x /etc/fs/$hint/fstyp ]
then
	/etc/fs/$hint/fstyp ${rrootfs} >/dev/null 2>&1
	[ $? -eq 0 ] && fstyp=$hint
fi

# There is no point in passing the undocumented hint argument to /sbin/fstyp.
[ -z "$fstyp" ] && fstyp=`/sbin/fstyp ${rrootfs} 2>/dev/null`
if [ -z "$fstyp" ]
then
	fstypa=""
else
	fstypa="-F $fstyp"
fi

remount=no

donext()
{
	case $1 in
	  0|40)	# remount the root file system only if in compatibility
		if [ ! -z "$2" ]
		then
			remount=yes
		fi
		;;
	  101)  # undconditionally remount root file system
		remount=yes
		;;

	  39)	# couldn't fix root!
		pfmt -l $LABEL -s halt -g $CAT:187 "The root file system on your hard disk is corrupted\nand cannot be repaired using the standard procedures and utilities.\n\n*** The root file system must be repaired or replaced before proceeding. ***\n\n"
		pfmt -s nostd -g $CAT:188 "There are several options available:\n\n"
		pfmt -s nostd -g $CAT:189 "	-- Use your Emergency Recovery Floppy and associated utilities to\n	   repair the existing root file system.\n\n"
		pfmt -s nostd -g $CAT:190 "	-- Use your Emergency Recovery Floppy and Emergency Recovery Tape\n	   to restore the system to its initial state, and then restore your\n	   data from your personal backup archive.\n\n"
		pfmt -s nostd -g $CAT:191 "	-- Contact appropriate support personnel to determine if the problem\n	   with your hard disk can be corrected.\n\n"
		pfmt -s nostd -g $CAT:192 "	-- Reinstall UnixWare from the original media and then restore\n	   your data from your personal backup archive.\n\n"
		while true
		do
			sleep 6000
		done
		;;

	  *)	# fsck determined reboot is necessary
		pfmt -l $LABEL -s warn -g $CAT:51 "return value %s\n" $1
		pfmt -l $LABEL -s info -g $CAT:52 "  *** SYSTEM WILL REBOOT ***\n"
		uadmin 1 0
		;;
	
	esac
}

prt_msg=0

/sbin/fsck $fstypa -m ${rrootfs}  >/dev/null 2>&1

retval=$?
if [ $retval -eq 101 ]
then
	remount="yes"
elif [ $retval -ne 0 ]
then
	pfmt -l $LABEL -s info -g $CAT:53 "\nPlease wait while the system is examined.  This may take a few minutes.\n\n"
	prt_msg=1
	/sbin/fsck $fstypa -y ${rrootfs} > /dev/null 2>&1
	donext $? "compat"
fi

if [ ! -z "$fstyp" -a -x /etc/fs/$fstyp/ckroot ]
then
	/etc/fs/$fstyp/ckroot
	donext $?
fi

if [ $remount = "yes" ]
then
	/sbin/uadmin 4 0 
	if [ $? -ne 0 ]
	then
		pfmt -l $LABEL -s error -g $CAT:54 "*** REMOUNT OF ROOT FAILED ***\n"
		pfmt -l $LABEL -s info -g $CAT:55 "*** SYSTEM WILL REBOOT AUTOMATICALLY ***\n"
		/sbin/uadmin 2 1
	fi
	#pfmt -l $LABEL -s info -g $CAT:56 "  *** ROOT REMOUNTED ***\n"
fi

if [ $prt_msg -eq 1 ]
then
	echo >/etc/.fscklog	# fsck message has been printed
else
	/sbin/rm /etc/.fscklog >/dev/null 2>&1
fi

/sbin/setmnt "${rootfs} ${rootmntp}" </dev/null >/dev/null 2>&1

# If the hint was wrong, update it for future use.

exec 2>/dev/null
umask 022
[ -n "$fstyp" -a "$fstyp" != "$hint" ] && echo $fstyp >$idcache
