#!/sbin/sh
#ident	"@(#)initpkg:common/cmd/initpkg/rc3.sh	1.11.11.11"

#	"Run Commands" executed when the system is changing to init state 3,
#	same as state 2 (multi-user) but with distributed file system support.

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

set -- `LC_ALL=C /sbin/who -r`
_CURR_RL=$7 _CURR_NTIMES=$8 _PREV_RL=$9 export _CURR_RL _CURR_NTIMES _PREV_RL
case "$_PREV_RL" in
s | S )
	if [ "$_CURR_RL" -eq 3 -a "$_CURR_NTIMES" -eq 0 ]
	then
		_AUTOBOOT=true export _AUTOBOOT
	fi
	;;
esac

cd /etc/rc3.d 2>/dev/null
if [ $? -eq 0 ]
then
	for f in K*
	{
		case $f in
		K\* ) ;;
		* ) /sbin/sh ${f} stop ;;
		esac
	}
	for f in S*
	{
		case $f in
		S\* ) ;;
		* ) /sbin/sh ${f} start ;;
		esac
	}
fi
if [ $_PREV_RL = 'S' -o $_PREV_RL = '1' ]
then
	pfmt -l $LABEL -s info -g $CAT:79 "The system is ready.\n"
fi
