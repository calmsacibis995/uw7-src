#!/sbin/sh
#ident	"@(#)initpkg:i386/cmd/initpkg/rc0.sh	1.15.21.15"

#	"Run Commands" for init state 0, 5 or 6
#	Leaves the system in a state where it is safe to turn off the power
#	or reboot. 
#
#	Takes an optional argument of off, firm or reboot
#	to specify if this is being run for init 0, init 5, or init 6.
#
#	In SVR4.0, inittab has been changed to no longer do the
#	uadmin to shutdown or enter firmware.  Instead, this script
#	is responsible.  By using an optional argument,
#	compatibility is maintained while still providing the needed
#	functionality to perform the uadmin call.

# make sure /usr is mounted before proceeding since init scripts
# and this shell depend on things on /usr file system
if [ ! -d /usr/bin ]
then
	/sbin/mount /usr >/dev/null 2>&1
	/sbin/initprivs 2> /dev/null
fi

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

CAT=uxrc; export CAT

ARG="$1"  # save arg for later

set -- `LC_ALL=C /sbin/who -r`
_CURR_RL=$7 _CURR_NTIMES=$8 _PREV_RL=$9 export _CURR_RL _CURR_NTIMES _PREV_RL
if [ "$ARG" = shutdown_s ]
then
	# We are run from shutdown.

	_AUTOKILL=true export _AUTOKILL
else
	# We are run from inittab.

	case "$_PREV_RL" in
	2 | 3 )
		case "$_CURR_RL" in
		0 | 5 | 6 )
			AUTOKILL=true export _AUTOKILL
			;;
		esac
	esac
fi

pfmt -l $LABEL -s info -g $CAT:71 "The system is coming down.  Please wait.\n"

#INFO messages should not appear in screen, save them in shut.log
exec 4>&2 3>&1 2>/dev/null >/var/adm/shut.log
exec 2>&4
chmod 664 /var/adm/shut.log

umask 022

#	The following segment is for historical purposes.
#	There should be nothing in /etc/shutdown.d.
cd /etc/shutdown.d 2>/dev/null
if [ $? -eq 0 ]
then
	for f in *
	{
		case $f in
		\* ) ;;
		* ) /sbin/sh ${f} ;;
		esac
	}
fi
#	End of historical section

cd /etc/rc0.d 2>/dev/null
if [ $? -eq 0 ]
then
	for f in K*
	{
		case $f in
		K\* )
			;;
		* )
			/sbin/sh ${f} stop
			;;
		esac
	}

	#	system cleanup functions ONLY (things that end fast!)	

	for f in S*
	{
		case $f in
		S\* ) ;;
		* ) /sbin/sh ${f} start ;;
		esac
	}
fi

# Note that S* scripts had better terminate synchronously, or they will
# get caught here.

/usr/sbin/killall&

# PC 6300+ Style Installation - execute shutdown scripts from driver packages
cd /etc/idsd.d 2>/dev/null
if [ $? -eq 0 ]
then
	for f in *
	{
		case $f in
		\* ) ;;
		* ) /sbin/sh ${f} ;;
		esac
	}
fi
exec 1>&3

trap "" 15
if [ -f /etc/conf/.copy_unix -a -f /etc/conf/cf.d/unix ]
then
	/etc/conf/bin/idcpunix
fi

wait	# for killall

# Check if user wants machine brought down or reboot.
# umountall will be done in uadmin if we are shutting down.
case "$ARG" in
	off)
		/sbin/uadmin 2 0
		;;

	firm)
		/sbin/uadmin 2 2
		;;

	reboot)
		/sbin/uadmin 2 1
		;;
	* )
		sleep 10	# for killall to work
		/sbin/umountall >/dev/null 2>&1
		;;
esac

pfmt -l $LABEL -s warn -g $CAT:72 "\n\tUser level file systems may still be mounted.\n\tMake sure to umount those file systems if you\n\tare going to powerdown the system. Otherwise,\n\tthe file systems may be corrupted.\n\n"
