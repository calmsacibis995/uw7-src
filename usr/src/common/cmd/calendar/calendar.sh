#!/sbin/sh
#	Copyright (c) 1984, 1986, 1987, 1988, 1989 AT&T
#	  All Rights Reserved

#	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF AT&T
#	The copyright notice above does not evidence any
#	actual or intended publication of such source code.

#ident	"@(#)calendar:calendar.sh	1.8.3.4"
#	calendar.sh - calendar command, uses /usr/lib/calprog

PATH=/usr/bin
_tmp=/tmp/cal$$
pwd_tmp=/tmp/pwd$$
trap "rm -f ${_tmp} ${pwd_tmp} /tmp/calendar.$$; trap '' 0; exit" 0 1 2 13 15
/usr/lib/calprog > ${_tmp}
/usr/bin/cat /etc/passwd | sed 's/\([^:]*\):.*:\(.*\):[^:]*$/_dir=\2 _user=\1/' > ${pwd_tmp}
if [ -f /usr/bin/ypcat ]; then
   /usr/bin/ypwhich >/dev/null 2>&1
   if [ $? -eq 0 ]; then	
	/usr/bin/ypcat -t passwd.byname | sed 's/\([^:]*\):.*:\(.*\):[^:]*$/_dir=\2 _user=\1/' >> ${pwd_tmp}
   fi
fi
/usr/bin/sort -d -u -o ${pwd_tmp} ${pwd_tmp}
case $# in
0)	if [ -f calendar ]; then
		egrep -f ${_tmp} calendar
	else
#		echo $0: `pwd`/calendar not found
		pfmt -l "UX:calendar" -s error -g uxue:318 "%s/calendar not found\n" `pwd`
	fi;;
*)	cat ${pwd_tmp} | \
		sed 's/\([^:]*\):.*:\(.*\):[^:]*$/_dir=\2 _user=\1/' | \
		while read _token; do
			eval ${_token}	# evaluates _dir= and _user=
			if [ -s ${_dir}/calendar ]; then
				egrep -f ${_tmp} ${_dir}/calendar 2>/dev/null \
					> /tmp/calendar.$$
				if [ -s /tmp/calendar.$$ ]; then
					mail ${_user} < /tmp/calendar.$$
				fi
			fi
		done;;
esac
exit 0
