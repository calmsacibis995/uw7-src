#!/sbin/sh
#	copyright	"%c%"

#ident	"@(#)tcpio:rtcpio.sh	1.2.2.2"
#ident  "$Header$"
#########################################################################
#	Module Name: rtcpio.sh  					#
#	Restricted tcpio.						#
#									#
#	Calls tcpio, restricting level range of files to be backedup	#
#	or restored.							#
#	Lower limit is USER_PUBLIC					#
#	Upper limit is USER_LOGIN.					#
#									#
#########################################################################

CMDPATH="/usr/lib/rtcpio/"
TCPIOARGS=$*
MINUSX=FALSE
RTCPIOMAX=USER_LOGIN #future enhancement: make this configureable
RTCPIOMIN=USER_PUBLIC
CATALOG=uxcore.abi
LABEL=UX:rtcpio
# define usage message function
showusage(){
   pfmt -l $LABEL -s action "\n"
   gettxt $CATALOG:987 "Usage:\trtcpio -i -I file [-bdfkPrsStuvVx] [-C size] [-E file]\n"
   gettxt $CATALOG:988 "\t  [-M msg] [-n num] [-N level] [-R ID] [-T file]\n"
   gettxt $CATALOG:989 "\t  [-X lo,hi] [patterns]\n"
   gettxt $CATALOG:990 "\trtcpio -o -O file [-aLvVx] [-C size] [-M msg] [-X lo,hi]\n"

   exit 1;
}
# validate options.  for -X option, process args
while getopts oiabC:dE:fI:kLMn12345N:O:rR:sStT:uvVxX:P option
do
  case "$option" in
   	X) MINUSX=TRUE
	   LVLRANGE=$OPTARG
	   if [ `echo $OPTARG | tr ',' ' ' |wc -w` -ne 2 ]
	      then 
		pfmt -l $LABEL -g $CATALOG:917 -s error -- "-X: syntax"
		showusage
	   fi 
	   LOW=`echo $OPTARG | sed 's/,.*$//'`
	   HIGH=`echo $OPTARG | sed 's/^.*,//'` ;;
	\?) showusage ;;
	esac
done
if [ "$MINUSX" != TRUE ]
then
	tcpio -X $RTCPIOMIN,$RTCPIOMAX $*
else # -X exists: validate levels, check that they are in allowed range.
	for LVL in $LOW $HIGH
	do
		${CMDPATH}vallvl $LVL
		if [ $? -ne 0 ]
		then
			pfmt -l $LABEL -g $CATALOG:991 -s error "Invalid level, %s.\n" $LVL
			exit 1
		fi
	done

	${CMDPATH}lvldom $LOW $RTCPIOMIN
	VALLOW=$?
	${CMDPATH}lvldom $RTCPIOMAX $HIGH
	if [ $VALLOW -eq 0 -a $? -eq 0 ]
	then
		tcpio $*
	else
		pfmt -l $LABEL -g $CATALOG:992 -s error -- "-X option: range must be within %s,%s.\n" $RTCPIOMIN $RTCPIOMAX
		exit 1
	fi
fi
