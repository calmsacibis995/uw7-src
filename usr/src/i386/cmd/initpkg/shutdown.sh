#!/sbin/sh
#ident	"@(#)initpkg:i386/cmd/initpkg/shutdown.sh	1.8.21.3"
#ident  "$Header$"

if [ -z "$LC_ALL" -a -z "$LC_MESSAGES" ]
then
	if [ -z "$LANG" ]
	then
		LNG=`/usr/bin/defadm locale LANG 2>/dev/null`
		if [ "$?" != 0 ]
		then LANG=C
		else eval $LNG
		fi
	fi
	# if catalogs aren't under /usr/lib/locale, check /etc/inst/locale
	if [ -d /usr/lib/locale/$LANG ] 
	then LC_MESSAGES=$LANG
	else LC_MESSAGES=/etc/inst/locale/$LANG
	fi
	export LANG LC_MESSAGES
fi
LABEL="UX:shutdown"
CAT="uxshutdown"

#	Sequence performed to change the init state of a machine.

#	This procedure checks to see if you are permitted and allows an
#	interactive shutdown.  The actual change of state, killing of
#	processes and such are performed by the new init state, say 0,
#	and its /sbin/rc0.

#	Usage:  shutdown [ -y ] [ -g<grace-period> ] [ -i<init-state> ]

priv -allprivs +sysops work	# turn off all working privileges

if [ `/usr/bin/pwd` != / ]
then
	/usr/bin/pfmt -l $LABEL -s error -g $CAT:4 "You must be in the / directory to run /sbin/shutdown.\n"
	exit 1
fi

# Make sure /usr is mounted
if [ ! -d /usr/bin ]
then
	/usr/bin/pfmt -l $LABEL -s error -g $CAT:5 "/usr is not mounted.  Mount /usr or use init to shutdown.\n"
	exit 1
fi

askconfirmation=yes

if /sbin/i386
then
	initstate=0
else
	initstate=s
fi

while getopts ?yg:i: c
do
	case $c in
	i)	initstate=$OPTARG; 
		case $initstate in
		2|3|4)
			/usr/bin/pfmt -l $LABEL -s error -g $CAT:8 "Initstate %s is not for system shutdown.\n" $initstate
			exit 1
		esac
		;;
	g)	grace=$OPTARG; 
		;;
	y)	askconfirmation=
		;;
	\?)	/usr/bin/pfmt -l $LABEL -s action -g $CAT:9 "Usage:  %s [ -y ] [ -g<grace> ] [ -i<initstate> ]\n" $0
		exit 2
		;;
	*) 	
		/usr/bin/pfmt -l $LABEL -s action -g $CAT:9 "Usage:  %s [ -y ] [ -g<grace> ] [ -i<initstate> ]\n" $0
		exit 2
		;;
	esac
done
if [ -z "$grace" ]
then
	if [ -r /etc/default/shutdown ]
	then
		grace=`/usr/bin/grep grace /etc/default/shutdown`
		if [ "${grace}" = "" ]
		then
			/usr/bin/pfmt -l $LABEL -s warn -g $CAT:6 "Could not read /etc/default/shutdown.\n"
			/usr/bin/pfmt -s nostd -g $CAT:7 "\tSetting default grace period to 60 seconds.\n"
			grace=60
		else
			eval $grace
		fi
	else
		/usr/bin/pfmt -l $LABEL -s warn -g $CAT:6 "Could not read /etc/default/shutdown.\n"
		/usr/bin/pfmt -s nostd -g $CAT:7 "\tSetting default grace period to 60 seconds.\n"
		grace=60
	fi
fi

shift `/usr/bin/expr $OPTIND - 1`

if [ -x /usr/alarm/bin/event ]
then
	/usr/alarm/bin/event -c gen -e shutdown -- -t $grace
fi

if [ -z "${TZ}"  -a  -r /etc/TIMEZONE ]
then
	. /etc/TIMEZONE
fi

/usr/bin/pfmt -l $LABEL -s info -g $CAT:10 '\nShutdown started.    ' 2>&1
/usr/bin/date
echo

/sbin/sync&

trap "exit 1"  1 2 15

WALL()
{
	SAVE_LC=$LC_ALL
	LC_ALL=`/usr/bin/defadm locale LANG | /usr/bin/cut -d= -f2` 
	export LC_ALL
	/usr/bin/pfmt -l UX:shutdown -s warn "$@" 2>&1 | /usr/sbin/wall
	LC_ALL=$SAVE_LC
}

set -- `/sbin/who`
if [ $? -eq 0 -a $# -gt 5  -a  ${grace} -gt 0 ]
then
	priv +macwrite +dacwrite +dev work
	WALL -g uxshutdown:1 "The system will be shut down in %s seconds.\nPlease log off now.\n\n" ${grace}
	priv -allprivs +sysops work
	/usr/bin/sleep ${grace} 2>/dev/null
	if [ $? -eq 2 ]
	then
		/usr/bin/pfmt -l $LABEL -s error -g $CAT:11 "Invalid grace period provided.\n"
		/usr/bin/pfmt -l $LABEL -s action -g $CAT:9 "Usage:  %s [ -y ] [ -g<grace> ] [ -i<initstate> ]\n" $0
		exit 2
	fi
	priv +macwrite +dacwrite +dev work

	WALL -g uxshutdown:2 "THE SYSTEM IS BEING SHUT DOWN NOW ! ! !\nLog off now or risk your files being damaged.\n\n"
	priv -allprivs +sysops work
fi	

if [ ${grace} -ne 0 ]
then
	/usr/bin/sleep ${grace} 2>/dev/null
fi
if [ $? -eq 2 ]
then
	/usr/bin/pfmt -l $LABEL -s error -g $CAT:11 "Invalid grace period provided.\n"
	/usr/bin/pfmt -l $LABEL -s action -g $CAT:9 "Usage:  %s [ -y ] [ -g<grace> ] [ -i<initstate> ]\n" $0
	exit 2
fi

y=`/usr/bin/gettxt $CAT:12 "y"`
n=`/usr/bin/gettxt $CAT:13 "n"`
if [ "${askconfirmation}" ]
then
	/usr/bin/pfmt -s nostd -g $CAT:14 "Do you want to continue? (%s or %s):   " $y $n 2>&1
	read b
else
	b=$y
fi
if [ "$b" != "$y" ]
then
	priv +macwrite +dacwrite +dev work
	WALL -g uxshutdown:3 "False Alarm:  The system will not be brought down.\n"
	priv -allprivs +sysops work
	/usr/bin/pfmt -l $LABEL -s info -g $CAT:15 'Shut down aborted.\n'
	exit 1
fi
case "${initstate}" in
s | S )
	priv +allprivs work
	. /sbin/rc0 shutdown_s
	priv -allprivs +sysops work
esac

RET=0
priv +owner +compat work
/sbin/init ${initstate}
RET=$?
priv -allprivs +sysops work
exit ${RET}
