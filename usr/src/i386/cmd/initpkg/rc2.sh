#!/sbin/sh

#ident	"@(#)initpkg:i386/cmd/initpkg/rc2.sh	1.16.22.1"

#	"Run Commands" executed when the system is changing to init state 2,
#	traditionally called "multi-user".

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

. /etc/TIMEZONE

#
#  update the device table and the contents file for PDI devices
#

if [ -s /etc/scsi/installf.input ]
then
	pfmt -l $LABEL -s info -g $CAT:195 "\nPlease wait while Device Database is updated.  This may take a few minutes.\n\n"
	/etc/scsi/pdimkdtab -fi 2>/etc/scsi/dtab.out
fi

#
#  backup the device table files in case of corruption
#

if [ \! -f /etc/.device.tab ]
then
	cp /etc/device.tab /etc/.device.tab >/dev/null 2>&1
fi

if [ \! -f /etc/security/ddb/.ddb_dsfmap ]
then
	cp /etc/security/ddb/ddb_dsfmap /etc/security/ddb/.ddb_dsfmap >/dev/null 2>&1
fi

if [ \! -f /etc/security/mac/.ltf.alias ]
then
	cp /etc/security/mac/ltf.alias /etc/security/mac/.ltf.alias >/dev/null 2>&1
fi

#
#	Pickup start-up packages for mounts, daemons, services, etc.
#

set -- `LC_ALL=C /sbin/who -r`
_CURR_RL=$7 _CURR_NTIMES=$8 _PREV_RL=$9 export _CURR_RL _CURR_NTIMES _PREV_RL
unset _AUTOBOOT _AUTOKILL _AUTOUMOUNT
case "$_PREV_RL" in
s | S )
	if [ \( "$_CURR_RL" -eq 2 -o "$_CURR_RL" -eq 3 \) \
	     -a "$_CURR_NTIMES" -eq 0 ]
	then
		_AUTOBOOT=true export _AUTOBOOT
	fi
	;;
esac

if [ x$_PREV_RL = "xS" -o x$_PREV_RL = "x1" ]
then
	BOOT=yes

elif [ x$_CURR_RL = "x2" ]
then
	pfmt -l $LABEL -s info -g $CAT:77 "Changing to state 2.\n"
	cd /etc/rc2.d 2>/dev/null
	if [ $? -eq 0 ]
	then
		for f in K*
		{
			case $f in
			K\* ) ;;
			* ) /sbin/sh ${f} stop ;;
			esac
		}
	fi
fi

cd /etc/rc2.d 2>/dev/null
if [ $? -eq 0 ]
then
	for f in S*
	{
		case $f in
		S\* ) ;;
		S01MOUNTFSYS )	/sbin/sh ${f} start
				[ -d /usr/lib ] && LC_MESSAGES=$LANG ;;
		* ) /sbin/sh ${f} start ;;
		esac
	}
fi

# Execute rc scripts from driver packages (386)
cd /etc/idrc.d 2>/dev/null
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

if [ "${BOOT}" = "yes" ]
then
	cd /etc/rc.d 2>/dev/null
	if [ $? -eq 0 ]
	then
		for f in *
		{
			case $f in
			\* ) ;;
			* ) [ ! -s /etc/init.d/${f} ] && /sbin/sh ${f} ;;
			esac
		}
	fi
	cd /
fi

if [ x$_CURR_RL = "x2" ]
then
	if [ "${BOOT}" = "yes" ]
	then
		# Enhanced Application Compatibility  Support
		/usr/bin/cat /etc/copyrights/*  2> /dev/null
		# End Enhanced Application Compatibility Support
	else
		pfmt -l $LABEL -s info -g $CAT:78 "Change to state 2 has been completed.\n"
	fi
fi
