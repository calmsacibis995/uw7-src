#!/sbin/sh
#	copyright	"%c%"

#ident	"@(#)acct:common/cmd/acct/remove.sh	1.6.1.3"
#ident "$Header$"
rm -f /var/adm/acct/sum/wtmp*
rm -f /var/adm/acct/sum/pacct*
rm -f /var/adm/acct/nite/lock*
