#!/sbin/sh
#	copyright	"%c%"

#ident	"@(#)sa:common/cmd/sa/sa1.sh	1.5.1.6"
#ident "$Header$"

# Privileges:	P_DEV
# Restrictions:
#		cd: none

priv -allprivs work
DATE=`date +%d`
ENDIR=/usr/lib/sa
DFILE=/var/adm/sa/sa$DATE
cd $ENDIR
if [ $# = 0 ]
then
	exec $ENDIR/sadc 1 1 $DFILE
else
	exec $ENDIR/sadc $* $DFILE
fi
