#!/sbin/sh
#	copyright	"%c%"

#ident	"@(#)acct:common/cmd/acct/shutacct.sh	1.6.1.4"
#ident "$Header$"
#	"shutacct [arg] - shuts down acct, called from /usr/sbin/shutdown"
#	"whenever system taken down"
#	"arg	added to /var/adm/wtmp to record reason, defaults to shutdown"
PATH=/usr/lib/acct:/usr/bin:/usr/sbin
_reason=${1-"acctg off"}
acctwtmp  "${_reason}"  >>/var/adm/wtmp
turnacct off
