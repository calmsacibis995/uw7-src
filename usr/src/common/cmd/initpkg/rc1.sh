#!/sbin/sh
#ident	"@(#)initpkg:common/cmd/initpkg/rc1.sh	1.4.13.11"

#	"Run Commands" executed when the system is changing to init state 1
#

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
set -- `LC_ALL=C /sbin/who -r`
_CURR_RL=$7 _CURR_NTIMES=$8 _PREV_RL=$9 export _CURR_RL _CURR_NTIMES _PREV_RL
case "$_PREV_RL" in
2 | 3 )
	if [ "$_CURR_RL" -eq 1 ]
	then
		_AUTOKILL=true export _AUTOKILL
	fi
	;;
esac
if [ "$_PREV_RL" = "S" ]
then
	pfmt -l $LABEL -s info -g $CAT:73 "The system is coming up for administration.  Please wait.\n"
	BOOT=yes

elif [ "$_CURR_RL" = "1" ]
then
	pfmt -l $LABEL -s info -g $CAT:74 "Changing to state 1.\n"
	cd /etc/rc1.d 2>/dev/null
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

cd /etc/rc1.d 2>/dev/null
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

# we know /usr is mounted because one of the rc1.d scripts calls mountall
/usr/sbin/killall

# demand unload of all unused modules
/sbin/modadmin -u 0 2>/dev/null

if [ "${BOOT}" = "yes" -a "$_CURR_RL" = "1" ]
then
	pfmt -l $LABEL -s info -g $CAT:75 "The system is ready for administration.\n"
elif [ "$_CURR_RL" = "1" ]
then
	pfmt -l $LABEL -s info -g $CAT:76 "Change to state 1 has been completed.\n"
fi
