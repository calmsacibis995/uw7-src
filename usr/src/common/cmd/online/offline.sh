#!/sbin/sh
#ident	"@(#)mp.cmds:common/cmd/online/offline.sh	1.1"
#	NAME:	offline
#
#	DESCRIPTION:	take specified processors offline
#
#	SYNOPSIS:	offline  [-v]  [processor_id ...]
#
#	NOTE: 		This command for compatibility only
#			please use psradm(1m)

if [ ${#}  -eq  0 ]
then
	psradm -f -a
exit
fi
VERBOSE=0
STATE=0

if [ $1 = "-v" ] 	# Check for the verbose flag
then
shift 1
if [ ${#}  -eq  0 ]	# just "offline -v"
then
	psradm -f -a 
	psrinfo
exit
fi
VERBOSE=1;
fi

for ENG in $* 		# Try to turn off each engine in the list
do
if [ $VERBOSE -eq 1 ]
then
	STATE=`psrinfo -s $ENG`	
	psrinfo  $ENG
fi

psradm -f  $ENG		# The actual offline
if [ $STATE -eq 1 ] 	# Print engine state only if was not already offline
	then
		psrinfo $ENG
fi
done
