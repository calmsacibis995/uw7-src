#!/sbin/sh
#ident	"@(#)mp.cmds:common/cmd/online/online.sh	1.1"
#	NAME:	online
#
#	DESCRIPTION:	take specified processors online
#
#	SYNOPSIS:	online  [-v]  [processor_id ...]
#
#	NOTE: 		This command for compatibility only
#			please use psradm(1m)
#
if [ ${#}  -eq  0 ]	# no arguments
then
	psradm -n
exit
fi
VERBOSE=0
STATE=1
if [ $1 = "-v" ] 	# Check for the verbose flag
then
shift 1
if [ ${#}  -eq  0 ]
then
	psradm -n 
	psrinfo
exit
fi
VERBOSE=1;
fi

for ENG in $* 		# Try to turn on each engine in the list
do
if [ $VERBOSE -eq 1 ]
then
	STATE=`psrinfo -s $ENG`
	if [ $? -ne 0 ]
	then
		exit $? 
	fi
	psrinfo  $ENG
fi

psradm -n  $ENG		# turn on the engine
if [ $? -ne 0 ]		# exit for loop if bad result
then
	exit $? 
fi

if [ $STATE -eq 0 ] 	# Print engine state only if was not already on
	then
		psrinfo $ENG
fi
done
