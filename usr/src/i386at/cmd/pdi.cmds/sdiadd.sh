#! /sbin/sh
#ident	"@(#)pdi.cmds:sdiadd.sh	1.1.1.1"
#ident	"$Header$"

#set -x

#
#	sdiadd/sdirm - user-level shell commands for
#			hot insertion and hot removal of SCSI devices 
#

#
#	First, some function definitions and initialization.
#

SDIADD=sdiadd
SDIRM=sdirm
PROGNAME=`basename $0`
label=UX:$PROGNAME
msgdb=uxpdi_hot

giveusage()
{
	if [ $ADDING = true ]
	then
		pfmt -l $label -s action -g $msgdb:15 "Usage: sdiadd [-n] disk_number\n"
	else
		pfmt -l $label -s action -g $msgdb:16 "Usage: sdirm [-n] disk_number\n"
	fi
}

REMOVING=`[ "$PROGNAME" = $SDIRM ] && echo true || echo false`
ADDING=`[ "$PROGNAME" = $SDIADD ] && echo true || echo false`
if [ $ADDING = $REMOVING ]
then
	pfmt -l $label -s error -g $msgdb:17 "Invalid command invocation -- %s\n" "$0";
	pfmt -l $label -s action -g $msgdb:18 "Try typing /etc/scsi/sdiadd -?\n";
	exit 2;
fi

N_ARG=""

#main()
while getopts n c
do
	case $c in
	n)
		N_ARG=-n
		;;
	*)	giveusage; exit 2;;
	
	esac
done

PDI_HOT=/etc/scsi/pdi_hot

shift `/bin/expr $OPTIND - 1`

[ $ADDING = true ] && $PDI_HOT -i $N_ARG $1 || $PDI_HOT -r $N_ARG $1

exit $?

