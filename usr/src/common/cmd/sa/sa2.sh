#!/sbin/sh
#	copyright	"%c%"

#ident	"@(#)sa:common/cmd/sa/sa2.sh	1.4.1.3"
#ident "$Header$"

DATE=`date +%d`
RPT=/var/adm/sa/sar$DATE
DFILE=/var/adm/sa/sa$DATE
ENDIR=/usr/bin
cd $ENDIR
$ENDIR/sar $* -f $DFILE > $RPT
find /var/adm/sa \( -name 'sar*' -o -name 'sa*' \) -mtime +7 -exec rm {} \;
