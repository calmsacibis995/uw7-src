#!/sbin/sh
#	copyright	"%c%"

#ident	"@(#)acct:common/cmd/acct/startup.sh	1.7.1.4"
#ident "$Header$"
#	"startup (acct) - should be called from /etc/rc"
#	"whenever system is brought up"
PATH=/usr/lib/acct:/usr/bin:/usr/sbin
acctwtmp "acctg on" >>/var/adm/wtmp
turnacct on
#	"clean up yesterdays accounting files"
rm -f /var/adm/acct/sum/wtmp*
rm -f /var/adm/acct/sum/pacct*
rm -f /var/adm/acct/nite/lock*
