#!/sbin/sh
#ident	"@(#)sa:common/cmd/sa/perf.sh	1.4.7.1"
#ident "$Header$"

if [ -n "$_AUTOBOOT" ]
then
	mldmode > /dev/null 2>&1
	if [ "$?" = "0" ]
	then
		exec su sys -c "/sbin/tfadmin /usr/lib/sa/sadc /var/adm/sa/sa`date +%d`"
	else
		exec su sys -c "/usr/lib/sa/sadc /var/adm/sa/sa`date +%d`"
	fi
fi
